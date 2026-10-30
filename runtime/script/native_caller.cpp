#include "../../include/nativeCaller.h"
#include "../../include/natives.h"
#include "native_table.hpp"
#include "../core/memory.hpp"
#include "bindings.hpp"

namespace ScriptHook {
namespace {
struct CallFrame { NativeContext context; uint64_t hash; };
CallFrame frames[NativeInvoker::MaxNesting];
size_t depth;
uint32_t active_invocations;
uintptr_t owner;
uint64_t empty_result[3];
uint32_t last_status = NativeCall_NotInitialized;
uint64_t last_hash;

void status(NativeCallStatus value, uint64_t hash = 0) {
    __atomic_store_n(&last_hash, hash, __ATOMIC_RELAXED);
    __atomic_store_n(&last_status, uint32_t(value), __ATOMIC_RELEASE);
}
uint64_t* empty() { for (auto& value : empty_result) value = 0; return empty_result; }

NativeCallStatus script_context(uintptr_t tls) {
    if (!Memory::s_mainBase) return NativeCall_NotInitialized;
    if (!tls || tls > UINT64_MAX - binding::script_tls_active) return NativeCall_NoScriptContext;
    uintptr_t thread = 0, active = 0; uint8_t running = 0;
    if (!Memory::Read(tls + binding::script_tls_thread, thread) || !thread ||
        !Memory::Read(tls + binding::script_tls_active, running) || running != 1 ||
        !Memory::Read(Memory::s_mainBase + binding::active_script_thread, active) || active != thread ||
        !Memory::IsReadable(thread, 0x20)) return NativeCall_NoScriptContext;
    return NativeCall_Ok;
}

NativeCallStatus string_pointer(uintptr_t address) {
    constexpr size_t maximum = 16384;
    size_t scanned = 0;
    while (scanned < maximum) {
        if (address > UINT64_MAX - scanned) return NativeCall_InvalidPointer;
        const uintptr_t current = address + scanned;
        MemoryInfo info{}; uint32_t page = 0;
        if (sh_query_memory(&info, &page, current) || !(info.permission & 1) ||
            current < info.address || info.address > UINT64_MAX - info.size ||
            current >= info.address + info.size) return NativeCall_InvalidPointer;
        size_t bytes = info.address + info.size - current;
        if (bytes > maximum - scanned) bytes = maximum - scanned;
        const auto* text = reinterpret_cast<const char*>(current);
        for (size_t i = 0; i < bytes; ++i) if (!text[i]) return NativeCall_Ok;
        scanned += bytes;
    }
    return NativeCall_StringTooLong;
}

NativeCallStatus validate_arguments(const NativeContext& context) {
    if (context.m_pReturn != context.m_returns || context.m_pArgs != context.m_args ||
        context.m_nArgCount > NativeContext::MaxArgs || context.m_nDataCount) return NativeCall_InvalidContext;
    for (uint32_t i = 0; i < context.m_nArgCount; ++i) {
        const auto kind = context.m_pointerKinds[i];
        if (kind > NativeContext::VectorPointer) return NativeCall_InvalidContext;
        if (kind == NativeContext::Value || !context.m_args[i]) continue;
        const uintptr_t pointer = context.m_args[i];
        if (kind == NativeContext::StringPointer) {
            const auto result = string_pointer(pointer);
            if (result != NativeCall_Ok) return result;
        } else {
            const auto bytes = context.m_pointerBytes[i];
            if (!bytes || !Memory::IsReadable(pointer, bytes) ||
                ((kind == NativeContext::WritePointer || kind == NativeContext::VectorPointer) &&
                 !Memory::IsWritable(pointer, bytes))) return NativeCall_InvalidPointer;
        }
    }
    return NativeCall_Ok;
}

NativeCallStatus copy_vectors(NativeContext& context) {
    if (context.m_nDataCount > NativeContext::MaxVectors) return NativeCall_InvalidVectorCount;
    // Validate every destination before copying any temporary output. The
    // packed native temporaries are not plugin Vector3 objects.
    for (uint32_t i = 0; i < context.m_nDataCount; ++i) {
        if (!Memory::IsWritable(reinterpret_cast<uintptr_t>(context.m_pVectors[i]), sizeof(Vector3)))
            return NativeCall_InvalidPointer;
    }
    for (uint32_t i = context.m_nDataCount; i > 0; --i) {
        auto* destination = context.m_pVectors[i - 1];
        const auto& vector = context.m_vectorData[i - 1];
        destination->x = vector.x; destination->y = vector.y; destination->z = vector.z;
    }
    context.m_nDataCount = 0;
    return NativeCall_Ok;
}
}

void NativeInvoker::Initialize() {
    depth = owner = active_invocations = 0;
    for (auto& frame : frames) { frame.context.Reset(); frame.hash = 0; }
    (void)empty(); status(NativeCall_Ok);
}

bool NativeInvoker::Invoke(uint64_t hash, NativeContext& context) {
    if (context.m_active) { status(NativeCall_ReentrantContext, hash); return false; }
    const uintptr_t tls = sh_get_thread_pointer();
    const uintptr_t call_owner = __atomic_load_n(&owner, __ATOMIC_ACQUIRE);
    if (call_owner && call_owner != tls) { status(NativeCall_WrongThread, hash); return false; }
    context.ClearResults();
    NativeCallStatus result = context.m_status;
    if (result == NativeCall_Ok) result = script_context(tls);
    if (result == NativeCall_Ok) result = validate_arguments(context);
    NativeHandler handler = nullptr;
    if (result == NativeCall_Ok) {
        handler = NativeTable::GetHandler(hash);
        if (!handler) result = NativeCall_MissingHandler;
        else if (!Memory::IsExecutable(reinterpret_cast<uintptr_t>(handler)) ||
                 reinterpret_cast<uintptr_t>(handler) == Memory::s_mainBase + binding::missing_native)
            result = NativeCall_InvalidHandler;
    }
    if (result == NativeCall_Ok) {
        const uint32_t arguments = context.m_nArgCount;
        const size_t original_depth = depth;
        context.m_active = 1;
        ++active_invocations;
        handler(&context);
        --active_invocations;
        context.m_active = 0;
        if (depth != original_depth || __atomic_load_n(&owner, __ATOMIC_ACQUIRE) != call_owner) {
            // A handler that starts a raw call and returns without completing
            // it must not strand ownership or leave a ghost argument frame.
            depth = original_depth;
            __atomic_store_n(&owner, call_owner, __ATOMIC_RELEASE);
            result = NativeCall_InvalidContext;
        } else if (context.m_pReturn != context.m_returns || context.m_pArgs != context.m_args ||
            context.m_nArgCount != arguments) result = NativeCall_InvalidContext;
        else if (context.m_status != NativeCall_Ok) result = context.m_status;
        else result = copy_vectors(context);
    }
    if (result != NativeCall_Ok) context.ClearResults();
    context.m_status = result;
    status(result, hash);
    return result == NativeCall_Ok;
}

void NativeInvoker::Begin(uint64_t hash) {
    const uintptr_t tls = sh_get_thread_pointer();
    uintptr_t existing = __atomic_load_n(&owner, __ATOMIC_ACQUIRE);
    if (existing && existing != tls) { status(NativeCall_WrongThread, hash); return; }
    const auto valid = script_context(tls);
    if (valid != NativeCall_Ok) { status(valid, hash); return; }
    if (!existing && !__atomic_compare_exchange_n(&owner, &existing, tls, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        status(NativeCall_WrongThread, hash); return;
    }
    // A new Init replaces an unfinished argument list, as in the PC API.
    // Only a call made inside a currently executing handler consumes a frame.
    if (depth && !frames[depth - 1].context.m_active) {
        frames[depth - 1].context.Reset(); frames[depth - 1].hash = hash;
    } else {
        if (depth == MaxNesting) { status(NativeCall_DepthExceeded, hash); return; }
        auto& frame = frames[depth++]; frame.context.Reset(); frame.hash = hash;
    }
    status(NativeCall_Ok, hash);
}

NativeContext* NativeInvoker::CurrentContext() {
    const auto current_owner = __atomic_load_n(&owner, __ATOMIC_ACQUIRE);
    if (current_owner && current_owner != sh_get_thread_pointer()) { status(NativeCall_WrongThread); return nullptr; }
    if (!depth || !current_owner) { status(NativeCall_NoActiveCall); return nullptr; }
    if (frames[depth - 1].context.m_active) {
        if (getNativeCallStatus() != NativeCall_DepthExceeded) status(NativeCall_NoActiveCall);
        return nullptr;
    }
    return &frames[depth - 1].context;
}

uint64_t* NativeInvoker::End(uint64_t expectedHash) {
    auto* context = CurrentContext();
    if (!context) return empty();
    auto& frame = frames[depth - 1];
    if (expectedHash && expectedHash != frame.hash) context->Fail(NativeCall_HashMismatch);
    (void)Invoke(frame.hash, *context);
    uint64_t* result = context->m_returns;
    --depth;
    if (!depth) __atomic_store_n(&owner, uintptr_t(0), __ATOMIC_RELEASE);
    return result;
}
bool NativeInvoker::CanYield() { return !depth && !active_invocations && !owner; }
void NativeInvoker::AbandonCalls() {
    if (active_invocations) return;
    depth = 0; __atomic_store_n(&owner, uintptr_t(0), __ATOMIC_RELEASE);
}
void NativeInvoker::RestoreStatus(NativeCallStatus code, uint64_t hash) { status(code, hash); }
} // namespace ScriptHook

extern "C" void nativeInit(uint64_t hash) { ScriptHook::NativeInvoker::Begin(hash); }
extern "C" void nativePush64(uint64_t value) {
    if (auto* context = ScriptHook::NativeInvoker::CurrentContext()) context->PushRaw(value);
}
extern "C" uint64_t* nativeCall() { return ScriptHook::NativeInvoker::End(); }
extern "C" NativeCallStatus getNativeCallStatus() {
    return NativeCallStatus(__atomic_load_n(&ScriptHook::last_status, __ATOMIC_ACQUIRE));
}
extern "C" uint64_t getLastNativeHash() { return __atomic_load_n(&ScriptHook::last_hash, __ATOMIC_RELAXED); }
extern "C" bool nativeIsAvailable(uint64_t hash) {
    const auto handler = ScriptHook::NativeTable::GetHandler(hash);
    return handler && reinterpret_cast<uintptr_t>(handler) != ScriptHook::Memory::s_mainBase + binding::missing_native &&
           ScriptHook::Memory::IsExecutable(reinterpret_cast<uintptr_t>(handler));
}
extern "C" uint32_t getNativeCallerAbiVersion() { return SHV_NX_CALLER_ABI_VERSION; }
extern "C" bool nativeInvokeContext(uint64_t hash, void* context, uint32_t bytes) {
    if (bytes != sizeof(ScriptHook::NativeContext)) { ScriptHook::status(NativeCall_InvalidContext, hash); return false; }
    const uintptr_t address = reinterpret_cast<uintptr_t>(context);
    if (address % alignof(ScriptHook::NativeContext) || !ScriptHook::Memory::IsReadable(address, bytes) ||
        !ScriptHook::Memory::IsWritable(address, bytes)) {
        ScriptHook::status(NativeCall_InvalidPointer, hash); return false;
    }
    return ScriptHook::NativeInvoker::Invoke(hash, *static_cast<ScriptHook::NativeContext*>(context));
}
extern "C" void* nativeGetCallContext() { return ScriptHook::NativeInvoker::CurrentContext(); }
extern "C" uint64_t* nativeCallChecked(uint64_t hash) { return ScriptHook::NativeInvoker::End(hash); }

extern "C" bool nativeCallerSelfTest(NativeCallerSelfTestInfo* out) {
    using namespace ScriptHook;
    if (!Memory::IsWritable(reinterpret_cast<uintptr_t>(out), sizeof(*out))) {
        status(NativeCall_InvalidPointer); return false;
    }
    NativeCallerSelfTestInfo result{};
    const auto ready = script_context(sh_get_thread_pointer());
    if (ready != NativeCall_Ok) {
        result.failed = 1; result.last_error = ready; *out = result; status(ready); return false;
    }
    auto check = [&](bool condition) {
        const auto code = getNativeCallStatus();
        if (condition && code == NativeCall_Ok) ++result.passed;
        else {
            ++result.failed;
            result.last_error = code == NativeCall_Ok ? NativeCall_InvalidContext : code;
            result.last_error_hash = getLastNativeHash();
        }
    };
    (void)Natives::MISC::GET_GAME_TIMER(); check(true);
    const float frame = Natives::MISC::GET_FRAME_TIME(); check(frame >= 0.f && frame < 10.f);
    int32_t width = 0, height = 0;
    Natives::GRAPHICS::GET_SCREEN_RESOLUTION(&width, &height); check(width > 0 && height > 0);
    const Hash hash = Natives::MISC::GET_HASH_KEY("adder"); check(hash == 0xb779a091u);
    const auto vector = Natives::ENTITY::GET_ENTITY_COORDS(Entity(-1), false);
    check(vector.x == 0.f && vector.y == 0.f && vector.z == 0.f);
    const char* keyboard = Natives::MISC::GET_ONSCREEN_KEYBOARD_RESULT();
    check(!keyboard || Memory::IsReadable(reinterpret_cast<uintptr_t>(keyboard), 1));
    (void)Natives::HUD::IS_PAUSE_MENU_ACTIVE(); check(true);
    *out = result;
    status(result.last_error, result.last_error_hash);
    return !result.failed;
}
