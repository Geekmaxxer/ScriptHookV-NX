#pragma once
#include "types.h"
#include "main.h"
#include <type_traits>

namespace ScriptHook {
struct NativeVector { float x, y, z, padding; };
static_assert(sizeof(NativeVector) == 16);

class NativeContext {
public:
    static constexpr size_t MaxArgs = 64;
    static constexpr size_t MaxVectors = 4;
    enum PointerKind : uint8_t { Value, ReadPointer, WritePointer, StringPointer, VectorPointer };
    void* m_pReturn;
    uint32_t m_nArgCount, m_argPadding;
    void* m_pArgs;
    uint32_t m_nDataCount, m_dataPadding;
    Vector3* m_pVectors[MaxVectors];
    NativeVector m_vectorData[MaxVectors];
    uint64_t m_args[MaxArgs];
    uint64_t m_returns[3];
    uint8_t m_pointerKinds[MaxArgs];
    uint32_t m_pointerBytes[MaxArgs];
    NativeCallStatus m_status;
    uint32_t m_vectorArgs;
    uint32_t m_active;

    NativeContext() { Reset(); }
    void Reset() {
        m_pReturn = m_returns; m_pArgs = m_args;
        m_nArgCount = m_nDataCount = m_argPadding = m_dataPadding = m_vectorArgs = m_active = 0;
        m_status = NativeCall_Ok;
        ClearResults();
        for (size_t i = 0; i < MaxVectors; ++i) {
            m_pVectors[i] = nullptr;
            m_vectorData[i] = {0.f, 0.f, 0.f, 0.f};
        }
    }
    void ClearResults() { for (auto& value : m_returns) value = 0; }
    bool Fail(NativeCallStatus status) {
        if (m_status == NativeCall_Ok) m_status = status;
        return false;
    }
    bool PushRaw(uint64_t slot, PointerKind kind = Value, uint32_t bytes = 0) {
        if (m_status != NativeCall_Ok) return false;
        if (m_nArgCount >= MaxArgs) return Fail(NativeCall_ArgumentOverflow);
        const auto index = m_nArgCount++;
        m_args[index] = slot; m_pointerKinds[index] = kind; m_pointerBytes[index] = bytes;
        return true;
    }
    bool Push(const Vector3& vector) {
        if (m_nArgCount > MaxArgs - 3) return Fail(NativeCall_ArgumentOverflow);
        return Push(vector.x) && Push(vector.y) && Push(vector.z);
    }
    bool Push(const NativeNavData& data) {
        if (m_nArgCount > MaxArgs - 3) return Fail(NativeCall_ArgumentOverflow);
        return Push(data.slideToCoordHeading) && Push(data.maxSlopeNavigable) && Push(data.clampMaxSearchDistance);
    }
    template<class T> bool Push(T value) {
        using U = typename std::remove_cv<T>::type;
        if constexpr (std::is_same<U, Vector3>::value) {
            return Push(static_cast<const Vector3&>(value));
        } else if constexpr (std::is_same<U, NativeNavData>::value) {
            return Push(static_cast<const NativeNavData&>(value));
        } else if constexpr (std::is_same<U, decltype(nullptr)>::value) {
            return PushRaw(0);
        } else if constexpr (std::is_pointer<U>::value) {
            using P = typename std::remove_pointer<U>::type;
            using Plain = typename std::remove_cv<P>::type;
            static_assert(!std::is_same<Plain, bool>::value,
                          "Native BOOL* requires four-byte BOOL storage, not C++ bool*");
            static_assert(!std::is_function<P>::value, "Pass function pointers through the raw API explicitly");
            PointerKind kind = std::is_const<P>::value ? ReadPointer : WritePointer;
            uint32_t bytes = 1;
            if constexpr (!std::is_void<P>::value) bytes = sizeof(P);
            if constexpr (std::is_same<U, const char*>::value) kind = StringPointer;
            if constexpr (std::is_same<Plain, Vector3>::value) {
                static_assert(!std::is_const<P>::value, "Native vector references require writable Vector3 storage");
                if (value && m_vectorArgs >= MaxVectors) return Fail(NativeCall_InvalidVectorCount);
                if (value) ++m_vectorArgs;
                kind = VectorPointer;
            }
            return PushRaw(reinterpret_cast<uintptr_t>(value), kind, bytes);
        } else if constexpr (std::is_same<U, bool>::value) {
            return PushRaw(value ? 1 : 0);
        } else if constexpr (std::is_enum<U>::value) {
            return Push(static_cast<typename std::underlying_type<U>::type>(value));
        } else if constexpr (std::is_integral<U>::value && sizeof(U) < 4) {
            if constexpr (std::is_signed<U>::value) return PushRaw(uint32_t(int32_t(value)));
            else return PushRaw(uint32_t(value));
        } else {
            static_assert(std::is_integral<U>::value || std::is_same<U, float>::value,
                          "Native values require integer, float, enum, pointer or Vector3; use float literals for FLOAT arguments");
            static_assert(sizeof(U) <= sizeof(uint64_t));
            uint64_t slot = 0;
            const auto* from = reinterpret_cast<const uint8_t*>(&value);
            auto* to = reinterpret_cast<uint8_t*>(&slot);
            for (size_t i = 0; i < sizeof(U); ++i) to[i] = from[i];
            return PushRaw(slot);
        }
    }
    template<class T> static T Decode(const uint64_t* data) {
        static_assert(std::is_integral<T>::value || std::is_enum<T>::value ||
                      std::is_pointer<T>::value || std::is_same<T, float>::value ||
                      std::is_same<T, Vector3>::value, "Unsupported native return type");
        static_assert(sizeof(T) <= 24);
        T result{};
        if constexpr (std::is_same<T, bool>::value) {
            result = uint32_t(data[0]) != 0;
        } else if constexpr (std::is_same<T, Vector3>::value) {
            float* components[] = {&result.x, &result.y, &result.z};
            for (size_t i = 0; i < 3; ++i) {
                auto* to = reinterpret_cast<uint8_t*>(components[i]);
                const auto* from = reinterpret_cast<const uint8_t*>(data + i);
                for (size_t j = 0; j < 4; ++j) to[j] = from[j];
            }
        } else {
            const auto* from = reinterpret_cast<const uint8_t*>(data);
            auto* to = reinterpret_cast<uint8_t*>(&result);
            for (size_t i = 0; i < sizeof(T); ++i) to[i] = from[i];
        }
        return result;
    }
    template<class T> T GetResult() const { return Decode<T>(m_returns); }
};
static_assert(std::is_standard_layout<NativeContext>::value);
static_assert(offsetof(NativeContext, m_pReturn) == 0);
static_assert(offsetof(NativeContext, m_nArgCount) == 8);
static_assert(offsetof(NativeContext, m_pArgs) == 16);
static_assert(offsetof(NativeContext, m_nDataCount) == 24);
static_assert(offsetof(NativeContext, m_pVectors) == 32);
static_assert(offsetof(NativeContext, m_vectorData) == 64);
static_assert(offsetof(NativeContext, m_args) == 128);

class SHV_NX_API NativeInvoker {
public:
    static constexpr size_t MaxNesting = 8;
    static void Initialize();
    static bool Invoke(uint64_t hash, NativeContext& context);
    static void Begin(uint64_t hash);
    static NativeContext* CurrentContext();
    static uint64_t* End(uint64_t expectedHash = 0);
    // Internal scheduler boundary: never switch stacks inside a native handler
    // or with an unfinished raw call. Last-result status follows each script.
    static bool CanYield();
    static void AbandonCalls();
    static void RestoreStatus(NativeCallStatus code, uint64_t hash);
};
using ::nativeInit;
template<class T> inline void nativePush(T value) {
    if (auto* context = static_cast<NativeContext*>(::nativeGetCallContext())) context->Push(value);
}
template<class T> inline T nativeCall(uint64_t hash) {
    return NativeContext::Decode<T>(::nativeCallChecked(hash));
}
inline void nativeCallVoid(uint64_t hash) { (void)::nativeCallChecked(hash); }

// Each call owns its storage. Nested calls preserve their caller's arguments.
template<class R = void, class... Args> R invoke(uint64_t hash, Args... args) {
    NativeContext context;
    (context.Push(args), ...);
    (void)::nativeInvokeContext(hash, &context, sizeof(context));
    if constexpr (!std::is_void<R>::value) return context.GetResult<R>();
}
} // namespace ScriptHook
