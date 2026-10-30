#pragma once
#include "memory.h"

namespace ScriptHook::Memory {

// Explicit cxx_runtime profile, noexcept T. Locals are cleaned normally or by
// the owning runtime after cancellation. Get returns only an active borrow.
template<class T> class Managed {
public:
    Managed() noexcept = default;
    ~Managed() noexcept { (void)Reset(); }
    Managed(const Managed&) = delete;
    Managed& operator=(const Managed&) = delete;
    Managed(Managed&& other) noexcept : pointer_(other.pointer_), handle_(other.handle_) {
        other.pointer_ = nullptr; other.handle_ = 0;
    }
    Managed& operator=(Managed&& other) noexcept {
        if (this != &other && Reset()) {
            pointer_ = other.pointer_; handle_ = other.handle_;
            other.pointer_ = nullptr; other.handle_ = 0;
        }
        return *this;
    }

    template<class... Args> static Managed TryMake(Args&&... args) noexcept {
        Managed result;
        if (!scriptResourcesAvailable()) return result;
        T* value = TryNew<T>(std::forward<Args>(args)...);
        if (!value) return result;
        const auto handle = scriptResourceRegister(Destroy, value);
        if (!handle) { (void)Delete(value); return result; }
        result.pointer_ = value; result.handle_ = handle;
        return result;
    }
    T* Get() const noexcept { return handle_ && scriptResourceActive(handle_) ? pointer_ : nullptr; }
    explicit operator bool() const noexcept { return Get() != nullptr; }
    ShvNxResourceHandle Handle() const noexcept { return handle_; }
    bool Reset() noexcept {
        if (!handle_) return true;
        bool ok = true;
        if (scriptResourceActive(handle_)) {
            ok = scriptResourceRelease(handle_);
            if (!ok && scriptResourceActive(handle_)) return false;
        }
        pointer_ = nullptr; handle_ = 0;
        return ok;
    }
    T* Detach() noexcept {
        T* value = Get();
        if (value && !scriptResourceForget(handle_)) return nullptr;
        pointer_ = nullptr; handle_ = 0;
        return value;
    }

private:
    T* pointer_ = nullptr;
    ShvNxResourceHandle handle_ = 0;
    static void Destroy(void* context) noexcept {
        T* value = static_cast<T*>(context);
        (void)Delete(value);
    }
};
}
