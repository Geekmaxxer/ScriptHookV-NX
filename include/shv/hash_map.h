#pragma once
#include "containers.h"

namespace ScriptHook::Containers {

// Views borrow bytes only for the duration of a lookup. Embedded NUL bytes are
// significant; callers supply a valid readable span, including for empty keys.
struct StringKeyView {
    const char* data;
    size_t bytes;
};

inline size_t HashBytes(const char* data, size_t bytes) noexcept {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < bytes; ++i) {
        hash ^= static_cast<unsigned char>(data[i]);
        hash *= UINT64_C(1099511628211);
    }
    return static_cast<size_t>(hash);
}

template<class K> struct Hash {
    static_assert(std::is_integral_v<K> || std::is_enum_v<K>,
                  "Provide a stateless noexcept hash for this key type");
    size_t operator()(const K& key) const noexcept {
        uint64_t value = static_cast<uint64_t>(key);
        value ^= value >> 30;
        value *= UINT64_C(0xbf58476d1ce4e5b9);
        value ^= value >> 27;
        value *= UINT64_C(0x94d049bb133111eb);
        return static_cast<size_t>(value ^ (value >> 31));
    }
};

template<class K> struct Equal {
    bool operator()(const K& left, const K& right) const noexcept(noexcept(left == right)) {
        return left == right;
    }
};

template<> struct Hash<String> {
    size_t operator()(const String& key) const noexcept { return HashBytes(key.CStr(), key.Size()); }
    size_t operator()(StringKeyView key) const noexcept { return HashBytes(key.data, key.bytes); }
};

template<> struct Equal<String> {
    bool operator()(const String& left, const String& right) const noexcept {
        return (*this)(left, StringKeyView{right.CStr(), right.Size()});
    }
    bool operator()(const String& left, StringKeyView right) const noexcept {
        return left.Size() == right.bytes && (!right.bytes || !memcmp(left.CStr(), right.data, right.bytes));
    }
};

enum class HashInsertResult { Inserted, Exists, NoMemory, AliasedArgument };

// Linear probing with backward-shift erasure and a maximum 3/4 load. Storage
// belongs to the current plugin; no operation yields or supplies synchronization.
template<class K, class V, class Hasher = Hash<K>, class KeyEqual = Equal<K>>
class HashMap {
    struct Entry { K key; V value; };
    struct Slot {
        size_t hash;
        bool occupied;
        alignas(Entry) unsigned char storage[sizeof(Entry)];
        Entry* Get() noexcept { return std::launder(reinterpret_cast<Entry*>(storage)); }
        const Entry* Get() const noexcept { return std::launder(reinterpret_cast<const Entry*>(storage)); }
    };

    static_assert(std::is_nothrow_move_constructible_v<K> && std::is_nothrow_move_constructible_v<V>,
                  "HashMap requires noexcept key and value moves");
    static_assert(std::is_nothrow_destructible_v<K> && std::is_nothrow_destructible_v<V>,
                  "HashMap requires noexcept key and value destruction");
    static_assert(alignof(Slot) <= 4096, "Plugin heap alignment limit is 4096");
    static_assert(std::is_empty_v<Hasher> && std::is_empty_v<KeyEqual>,
                  "HashMap requires stateless hash and equality functors");
    static_assert(std::is_nothrow_default_constructible_v<Hasher> &&
                  std::is_nothrow_default_constructible_v<KeyEqual>,
                  "HashMap functor initialization must be noexcept");
    static_assert(std::is_nothrow_invocable_r_v<size_t, const Hasher&, const K&> &&
                  std::is_nothrow_invocable_r_v<bool, const KeyEqual&, const K&, const K&>,
                  "HashMap hash and equality must be noexcept");

    Slot* slots_{};
    size_t size_{}, buckets_{};
    Hasher hash_{};
    KeyEqual equal_{};

    static size_t Usable(size_t buckets) noexcept { return buckets - buckets / 4; }

    bool Aliases(const void* pointer) const noexcept {
        const auto first = reinterpret_cast<uintptr_t>(slots_);
        const auto at = reinterpret_cast<uintptr_t>(pointer);
        return slots_ && at >= first && at - first < buckets_ * sizeof(Slot);
    }

    template<class Q> size_t Locate(const Q& key, size_t hash) const noexcept {
        static_assert(std::is_nothrow_invocable_r_v<bool, const KeyEqual&, const K&, const Q&>,
                      "Lookup equality must be noexcept");
        if (!buckets_) return SIZE_MAX;
        const size_t mask = buckets_ - 1;
        size_t at = hash & mask;
        for (size_t visited = 0; visited < buckets_; ++visited) {
            const auto& slot = slots_[at];
            if (!slot.occupied) return SIZE_MAX;
            if (slot.hash == hash && equal_(slot.Get()->key, key)) return at;
            at = (at + 1) & mask;
        }
        return SIZE_MAX;
    }

    static Slot& Vacant(Slot* slots, size_t buckets, size_t hash) noexcept {
        size_t at = hash & (buckets - 1);
        while (slots[at].occupied) at = (at + 1) & (buckets - 1);
        return slots[at];
    }

    template<class A, class B> HashInsertResult Insert(A&& key, B&& value) noexcept {
        static_assert(std::is_nothrow_constructible_v<K, A&&> && std::is_nothrow_constructible_v<V, B&&>,
                      "Insertion requires noexcept key and value construction");
        const size_t hash = hash_(key);
        if (Locate(key, hash) != SIZE_MAX) return HashInsertResult::Exists;
        // Growth moves entries. Refuse borrowed inputs rather than consuming or
        // dereferencing them after rehash; duplicate keys above remain harmless.
        if (Aliases(std::addressof(key)) || Aliases(std::addressof(value)))
            return HashInsertResult::AliasedArgument;
        if constexpr (std::is_rvalue_reference_v<A&&> || std::is_rvalue_reference_v<B&&>) {
            if (static_cast<const void*>(std::addressof(key)) == static_cast<const void*>(std::addressof(value)))
                return HashInsertResult::AliasedArgument;
        }
        if (size_ == SIZE_MAX || !TryReserve(size_ + 1)) return HashInsertResult::NoMemory;
        auto& slot = Vacant(slots_, buckets_, hash);
        ::new(static_cast<void*>(slot.storage)) Entry{std::forward<A>(key), std::forward<B>(value)};
        slot.hash = hash;
        slot.occupied = true;
        ++size_;
        return HashInsertResult::Inserted;
    }

public:
    HashMap() noexcept = default;
    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;
    HashMap(HashMap&& other) noexcept { Swap(other); }
    HashMap& operator=(HashMap&& other) noexcept {
        if (this != &other) { Reset(); Swap(other); }
        return *this;
    }
    ~HashMap() noexcept { Reset(); }

    void Swap(HashMap& other) noexcept {
        auto* slots = slots_; slots_ = other.slots_; other.slots_ = slots;
        auto n = size_; size_ = other.size_; other.size_ = n;
        n = buckets_; buckets_ = other.buckets_; other.buckets_ = n;
    }
    size_t Size() const noexcept { return size_; }
    size_t Capacity() const noexcept { return Usable(buckets_); }
    size_t BucketCount() const noexcept { return buckets_; }

    bool TryReserve(size_t entries) noexcept {
        if (entries <= Capacity()) return true;
        constexpr size_t maximum = SIZE_MAX / sizeof(Slot);
        size_t buckets = buckets_ ? buckets_ : 8;
        if (buckets > maximum) return false;
        while (Usable(buckets) < entries) {
            if (buckets > maximum / 2) return false;
            buckets *= 2;
        }
        auto* next = static_cast<Slot*>(scriptAllocate(buckets * sizeof(Slot), alignof(Slot), 0));
        if (!next) return false;
        for (size_t i = 0; i < buckets; ++i) {
            ::new(static_cast<void*>(next + i)) Slot;
            next[i].occupied = false;
        }
        for (size_t i = 0; i < buckets_; ++i) if (slots_[i].occupied) {
            auto& destination = Vacant(next, buckets, slots_[i].hash);
            ::new(static_cast<void*>(destination.storage)) Entry(std::move(*slots_[i].Get()));
            destination.hash = slots_[i].hash;
            destination.occupied = true;
            slots_[i].Get()->~Entry();
        }
        if (slots_) (void)scriptFree(slots_);
        slots_ = next;
        buckets_ = buckets;
        return true;
    }

    HashInsertResult TryInsert(const K& key, const V& value) noexcept { return Insert(key, value); }
    HashInsertResult TryInsert(const K& key, V&& value) noexcept { return Insert(key, std::move(value)); }
    HashInsertResult TryInsert(K&& key, const V& value) noexcept { return Insert(std::move(key), value); }
    HashInsertResult TryInsert(K&& key, V&& value) noexcept { return Insert(std::move(key), std::move(value)); }

    template<class Q> const V* FindAs(const Q& key) const noexcept {
        static_assert(std::is_nothrow_invocable_r_v<size_t, const Hasher&, const Q&>,
                      "Lookup hash must be noexcept");
        const size_t at = Locate(key, hash_(key));
        return at == SIZE_MAX ? nullptr : std::addressof(slots_[at].Get()->value);
    }
    template<class Q> V* FindAs(const Q& key) noexcept {
        return const_cast<V*>(static_cast<const HashMap&>(*this).FindAs(key));
    }
    const V* Find(const K& key) const noexcept { return FindAs(key); }
    V* Find(const K& key) noexcept { return FindAs(key); }
    const K* KeyAt(size_t bucket) const noexcept {
        return bucket < buckets_ && slots_[bucket].occupied ? std::addressof(slots_[bucket].Get()->key) : nullptr;
    }
    const V* ValueAt(size_t bucket) const noexcept {
        return bucket < buckets_ && slots_[bucket].occupied ? std::addressof(slots_[bucket].Get()->value) : nullptr;
    }
    V* ValueAt(size_t bucket) noexcept {
        return const_cast<V*>(static_cast<const HashMap&>(*this).ValueAt(bucket));
    }

    template<class Q> bool EraseAs(const Q& key) noexcept {
        static_assert(std::is_nothrow_invocable_r_v<size_t, const Hasher&, const Q&>,
                      "Lookup hash must be noexcept");
        size_t hole = Locate(key, hash_(key));
        if (hole == SIZE_MAX) return false;
        slots_[hole].Get()->~Entry();
        slots_[hole].occupied = false;
        --size_;
        const size_t mask = buckets_ - 1;
        for (size_t at = (hole + 1) & mask; slots_[at].occupied; at = (at + 1) & mask) {
            const size_t home = slots_[at].hash & mask;
            if (((at - home) & mask) >= ((at - hole) & mask)) {
                ::new(static_cast<void*>(slots_[hole].storage)) Entry(std::move(*slots_[at].Get()));
                slots_[hole].hash = slots_[at].hash;
                slots_[hole].occupied = true;
                slots_[at].Get()->~Entry();
                slots_[at].occupied = false;
                hole = at;
            }
        }
        return true;
    }
    bool Erase(const K& key) noexcept { return EraseAs(key); }

    bool TryCopyFrom(const HashMap& other) noexcept {
        static_assert(std::is_nothrow_copy_constructible_v<K> && std::is_nothrow_copy_constructible_v<V>,
                      "CopyFrom requires noexcept key and value copies");
        if (this == &other) return true;
        HashMap next;
        if (!next.TryReserve(other.size_)) return false;
        for (size_t i = 0; i < other.buckets_; ++i) if (other.slots_[i].occupied) {
            auto& destination = Vacant(next.slots_, next.buckets_, other.slots_[i].hash);
            ::new(static_cast<void*>(destination.storage)) Entry(*other.slots_[i].Get());
            destination.hash = other.slots_[i].hash;
            destination.occupied = true;
            ++next.size_;
        }
        Swap(next);
        return true;
    }

    void Clear() noexcept {
        for (size_t i = 0; i < buckets_; ++i) if (slots_[i].occupied) {
            slots_[i].Get()->~Entry();
            slots_[i].occupied = false;
        }
        size_ = 0;
    }
    void Reset() noexcept {
        Clear();
        if (slots_) (void)scriptFree(slots_);
        slots_ = nullptr;
        buckets_ = 0;
    }
};
}
