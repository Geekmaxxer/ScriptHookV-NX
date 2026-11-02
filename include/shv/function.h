#pragma once
#include "memory.h"

namespace ScriptHook::Callbacks {

template<class Signature> class Function;

// Fallible, move-only void callable. Small targets are inline; larger or
// over-aligned targets use the owning heap. No RTTI, exceptions or std::function.
template<class... Args> class Function<void(Args...)> final {
    struct Operations {
        const void* type;
        void (*invoke)(void*, Args...);
        void (*destroy)(void*);
        void (*move)(void*, void*);
        bool (*copy)(Function&, const void*);
    };
    static constexpr size_t InlineBytes = 64;
    alignas(16) unsigned char storage_[InlineBytes]{};
    void* pointer_ = nullptr;
    const Operations* operations_ = nullptr;

    template<class T> static constexpr bool Inline = sizeof(T) <= InlineBytes && alignof(T) <= 16;
    template<class T> static void Invoke(void* context, Args... arguments) noexcept {
        (*static_cast<T*>(context))(std::forward<Args>(arguments)...);
    }
    template<class T> static void Destroy(void* context) noexcept { static_cast<T*>(context)->~T(); }
    template<class T> static void Move(void* destination, void* source) noexcept {
        ::new(destination) T(std::move(*static_cast<T*>(source)));
        static_cast<T*>(source)->~T();
    }
    template<class T> static bool Copy(Function& destination, const void* source) noexcept {
        if constexpr (std::is_nothrow_copy_constructible_v<T>)
            return destination.TryAssign(*static_cast<const T*>(source));
        else return false;
    }
    template<class T> inline static constexpr unsigned char Type = 0;
    template<class T> inline static constexpr Operations Table = {&Type<T>, Invoke<T>, Destroy<T>, Move<T>, Copy<T>};

    void MoveFrom(Function& source) noexcept {
        const auto* operations = source.operations_;
        if (!operations) return;
        void* pointer = source.pointer_;
        const bool local = pointer == source.storage_;
        source.operations_ = nullptr; source.pointer_ = nullptr;
        if (local) { operations->move(storage_, pointer); pointer_ = storage_; }
        else pointer_ = pointer;
        operations_ = operations;
    }

public:
    Function() noexcept = default;
    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;
    Function(Function&& source) noexcept { MoveFrom(source); }
    Function& operator=(Function&& source) noexcept {
        if (this != &source) { Reset(); MoveFrom(source); }
        return *this;
    }
    ~Function() noexcept { Reset(); }
    explicit operator bool() const noexcept { return operations_ != nullptr; }
    bool UsesHeap() const noexcept { return operations_ && pointer_ != storage_; }
    template<class F> bool TryAssign(F&& target) noexcept {
        using T = std::decay_t<F>;
        static_assert(!std::is_same_v<T, Function>, "Use move assignment or TryCopyFrom for Function");
        static_assert(!std::is_member_pointer_v<T>, "Wrap member pointers in a noexcept callable");
        static_assert(std::is_nothrow_constructible_v<T, F&&>, "Callable construction must be noexcept");
        static_assert(std::is_nothrow_move_constructible_v<T>, "Callable moves must be noexcept");
        static_assert(std::is_nothrow_destructible_v<T>, "Callable destruction must be noexcept");
        static_assert(std::is_nothrow_invocable_r_v<void, T&, Args...>, "Callable invocation must be noexcept");
        static_assert(alignof(T) <= 4096, "Plugin heap alignment limit is 4096");
        // Replacing a target with a borrow of itself would invalidate the input.
        if constexpr (std::is_object_v<std::remove_reference_t<F>>)
            if (static_cast<const void*>(std::addressof(target)) == pointer_) return false;
        if constexpr (std::is_pointer_v<std::remove_reference_t<F>>) {
            if (!target) { Reset(); return true; }
        }
        Function next;
        if constexpr (Inline<T>) next.pointer_ = next.storage_;
        else {
            next.pointer_ = scriptAllocate(sizeof(T), alignof(T), 0);
            if (!next.pointer_) return false;
        }
        ::new(next.pointer_) T(std::forward<F>(target));
        next.operations_ = &Table<T>;
        *this = std::move(next);
        return true;
    }
    bool TryCopyFrom(const Function& source) noexcept {
        if (this == &source) return true;
        if (!source.operations_) { Reset(); return true; }
        return source.operations_->copy(*this, source.pointer_);
    }
    bool TryInvoke(Args... arguments) noexcept {
        if (!operations_) return false;
        operations_->invoke(pointer_, std::forward<Args>(arguments)...);
        return true;
    }
    template<class T> T* Target() noexcept { return operations_ && operations_->type == &Type<T> ? static_cast<T*>(pointer_) : nullptr; }
    template<class T> const T* Target() const noexcept { return operations_ && operations_->type == &Type<T> ? static_cast<const T*>(pointer_) : nullptr; }
    void Reset() noexcept {
        const auto* operations = operations_;
        void* pointer = pointer_;
        operations_ = nullptr; pointer_ = nullptr;
        if (!operations) return;
        operations->destroy(pointer);
        if (pointer != storage_) (void)scriptFree(pointer);
    }
};
}
