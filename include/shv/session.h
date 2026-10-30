#pragma once
#include "memory.h"

namespace ScriptHook::Memory {

// One explicitly constructed object per type/tag and NRO. Access belongs to the
// runtime owner thread. A returned pointer is a borrow, never an owning pointer.
template<class T, class Tag = void> class SessionLocal final {
public:
    SessionLocal() = delete;

    static T* Get() noexcept {
        return handle_ && scriptResourceActive(handle_) ? pointer_ : nullptr;
    }

    template<class... Args> static T* TryGet(Args&&... args) noexcept {
        if (busy_) return nullptr;
        // A failed owner/thread check must not discard another caller's state.
        if (handle_) return Get();
        if (!scriptResourcesAvailable() || !scriptMemoryAvailable()) return nullptr;
        busy_ = true;
        T* value = TryNew<T>(std::forward<Args>(args)...);
        if (!value) { busy_ = false; return nullptr; }
        const auto handle = scriptResourceRegister(Destroy, value);
        if (!handle) {
            (void)Delete(value);
            busy_ = false;
            return nullptr;
        }
        pointer_ = value;
        handle_ = handle;
        busy_ = false;
        return value;
    }

    static bool Reset() noexcept {
        if (busy_) return false;
        if (!handle_) return true;
        // Release checks its preconditions before consumption. Destroy clears
        // the slot only once the runtime actually invokes the cleanup callback.
        return scriptResourceRelease(handle_);
    }

private:
    inline static T* pointer_ = nullptr;
    inline static ShvNxResourceHandle handle_ = 0;
    inline static bool busy_ = false;

    static void Destroy(void* context) noexcept {
        busy_ = true;
        pointer_ = nullptr;
        handle_ = 0;
        auto* value = static_cast<T*>(context);
        (void)Delete(value);
        busy_ = false;
    }
};
}
