#pragma once
#include "memory.h"

namespace ScriptHook::Containers {

// Owner-thread ring storage. Mutations and element operations must not yield
// or reenter this instance. Allocation failure preserves the existing queue.
template<class T> class Deque final {
    static_assert(std::is_nothrow_move_constructible_v<T>, "Deque requires noexcept moves");
    static_assert(std::is_nothrow_destructible_v<T>, "Deque requires noexcept destruction");
    static_assert(alignof(T) <= 4096, "Plugin heap alignment limit is 4096");
    T* data_ = nullptr;
    size_t size_ = 0, capacity_ = 0, head_ = 0;

    size_t Physical(size_t index) const noexcept {
        const size_t tail = capacity_ - head_;
        return index < tail ? head_ + index : index - tail;
    }
    size_t Alias(const T* pointer) const noexcept {
        if (!data_) return SIZE_MAX;
        const auto at = reinterpret_cast<uintptr_t>(pointer);
        const auto base = reinterpret_cast<uintptr_t>(data_);
        if (at < base || at - base >= capacity_ * sizeof(T) || (at - base) % sizeof(T)) return SIZE_MAX;
        const size_t physical = (at - base) / sizeof(T);
        const size_t logical = physical >= head_ ? physical - head_ : capacity_ - head_ + physical;
        return logical < size_ ? logical : SIZE_MAX;
    }
    bool Grow() noexcept {
        if (size_ < capacity_) return true;
        const size_t maximum = SIZE_MAX / sizeof(T);
        if (size_ == maximum) return false;
        size_t next = capacity_ > maximum / 2 ? maximum : capacity_ * 2;
        if (next < 8) next = maximum < 8 ? maximum : 8;
        return TryReserve(next);
    }
    template<class U> bool Push(U&& value, bool front) noexcept {
        const size_t alias = Alias(std::addressof(value));
        if (!Grow()) return false;
        const size_t target = front ? (head_ ? head_ - 1 : capacity_ - 1) : Physical(size_);
        if (alias != SIZE_MAX) ::new(static_cast<void*>(data_ + target)) T(std::forward<U>(*At(alias)));
        else ::new(static_cast<void*>(data_ + target)) T(std::forward<U>(value));
        if (front) head_ = target;
        ++size_;
        return true;
    }

public:
    Deque() noexcept = default;
    Deque(const Deque&) = delete;
    Deque& operator=(const Deque&) = delete;
    Deque(Deque&& other) noexcept { Swap(other); }
    Deque& operator=(Deque&& other) noexcept {
        if (this != &other) { Reset(); Swap(other); }
        return *this;
    }
    ~Deque() noexcept { Reset(); }
    void Swap(Deque& other) noexcept {
        auto* data = data_; data_ = other.data_; other.data_ = data;
        auto n = size_; size_ = other.size_; other.size_ = n;
        n = capacity_; capacity_ = other.capacity_; other.capacity_ = n;
        n = head_; head_ = other.head_; other.head_ = n;
    }
    size_t Size() const noexcept { return size_; }
    size_t Capacity() const noexcept { return capacity_; }
    bool Empty() const noexcept { return !size_; }
    T* At(size_t index) noexcept { return index < size_ ? data_ + Physical(index) : nullptr; }
    const T* At(size_t index) const noexcept { return index < size_ ? data_ + Physical(index) : nullptr; }
    T* Front() noexcept { return At(0); }
    const T* Front() const noexcept { return At(0); }
    T* Back() noexcept { return size_ ? At(size_ - 1) : nullptr; }
    const T* Back() const noexcept { return size_ ? At(size_ - 1) : nullptr; }
    bool TryReserve(size_t count) noexcept {
        if (count <= capacity_) return true;
        if (count > SIZE_MAX / sizeof(T)) return false;
        auto* next = static_cast<T*>(scriptAllocate(count * sizeof(T), alignof(T), 0));
        if (!next) return false;
        for (size_t i = 0; i < size_; ++i) {
            auto* old = At(i);
            ::new(static_cast<void*>(next + i)) T(std::move(*old));
            old->~T();
        }
        if (data_) (void)scriptFree(data_);
        data_ = next; capacity_ = count; head_ = 0;
        return true;
    }
    bool TryPushBack(T&& value) noexcept { return Push<T>(std::move(value), false); }
    bool TryPushFront(T&& value) noexcept { return Push<T>(std::move(value), true); }
    bool TryPushBack(const T& value) noexcept {
        static_assert(std::is_nothrow_copy_constructible_v<T>, "Copy push requires noexcept construction");
        return Push<const T&>(value, false);
    }
    bool TryPushFront(const T& value) noexcept {
        static_assert(std::is_nothrow_copy_constructible_v<T>, "Copy push requires noexcept construction");
        return Push<const T&>(value, true);
    }
    bool PopFront() noexcept {
        if (!size_) return false;
        Front()->~T();
        if (!--size_) head_ = 0;
        else head_ = head_ + 1 == capacity_ ? 0 : head_ + 1;
        return true;
    }
    bool PopBack() noexcept {
        if (!size_) return false;
        Back()->~T();
        if (!--size_) head_ = 0;
        return true;
    }
    bool TryCopyFrom(const Deque& other) noexcept {
        static_assert(std::is_nothrow_copy_constructible_v<T>, "Copy requires noexcept construction");
        if (this == &other) return true;
        Deque next;
        if (!next.TryReserve(other.size_)) return false;
        for (size_t i = 0; i < other.size_; ++i) {
            ::new(static_cast<void*>(next.data_ + i)) T(*other.At(i));
            ++next.size_;
        }
        Swap(next);
        return true;
    }
    void Clear() noexcept { while (PopBack()) {} }
    void Reset() noexcept {
        Clear();
        if (data_) (void)scriptFree(data_);
        data_ = nullptr; capacity_ = head_ = 0;
    }
};
}
