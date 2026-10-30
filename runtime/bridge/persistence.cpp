#include "persistence.hpp"
#include "../core/memory.hpp"
#include "../script/native_table.hpp"
#include "bindings.hpp"

namespace ScriptHook {
namespace {
constexpr size_t max_words = 2048;
constexpr uint32_t record_magic = 0x584e4853; // SHNX
struct ScriptContext { void* result; uint32_t count, padding; uint64_t* args; };
static_assert(offsetof(ScriptContext, count) == 8 && offsetof(ScriptContext, args) == 16);
struct Record { uint32_t magic, version, generation, bytes, crc; uint8_t data[max_words * 4]; };
static_assert(offsetof(Record, data) == 20);
template<class Fn> Fn game(uint32_t at) { return reinterpret_cast<Fn>(Memory::s_mainBase + at); }
template<class Fn> Fn sdk(uint32_t at) { return reinterpret_cast<Fn>(Memory::s_sdkBase + at); }
uint32_t last_result;
uint32_t save_calls, load_calls, save_successes, load_successes;
NativeCallerSelfTestInfo caller_selftest{};
bool caller_selftest_ran;
void write_report();

class Session {
    uintptr_t previous = 0;
    bool scoped = false;
public:
    bool mounted = false;
    Session() {
        if (!Memory::s_mainBase || !Memory::ResolveSdk()) return;
        sdk<void(*)(uintptr_t*)>(binding::disable_thread_abort)(&previous); scoped = true;
        last_result = sdk<uint32_t(*)(const char*)>(binding::mount_sd_debug)("megacfg");
        mounted = last_result == 0;
        if (mounted) {
            (void)game<uint32_t(*)(const char*)>(binding::create_directory)("megacfg:/switch");
            (void)game<uint32_t(*)(const char*)>(binding::create_directory)("megacfg:/switch/gtav-megatard");
        }
    }
    ~Session() {
        if (mounted) {
            write_report();
            sdk<void(*)(const char*)>(binding::unmount)("megacfg");
        }
        if (scoped) sdk<void(*)(uintptr_t)>(binding::restore_thread_context)(previous);
    }
};

bool path(char (&out)[128], const char* name) {
    constexpr char prefix[] = "megacfg:/switch/gtav-megatard/";
    if (!name) return false;
    size_t length = 0;
    for (; length < 64; ++length) {
        char c = 0;
        if (!Memory::Read(reinterpret_cast<uintptr_t>(name) + length, c)) return false;
        if (!c) break;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
        if (c == '.' && length && name[length - 1] == '.') return false;
    }
    if (!length || length == 64 || name[0] == '.') return false;
    size_t at = 0; for (; at < sizeof(prefix) - 1; ++at) out[at] = prefix[at];
    for (size_t i = 0; i < length; ++i) out[at++] = name[i];
    out[at] = 0; return true;
}

bool read_file(const char* name, void* out, size_t capacity, size_t& got) {
    got = 0; char full[128]{}; if (!path(full, name)) return false;
    uint64_t file = 0;
    last_result = game<uint32_t(*)(uint64_t*,const char*,int32_t)>(binding::open_file)(&file, full, 1);
    if (last_result) return false;
    last_result = game<uint32_t(*)(size_t*,uint64_t,int64_t,void*,size_t)>(binding::read_file)(&got, file, 0, out, capacity);
    game<void(*)(uint64_t)>(binding::close_file)(file);
    return !last_result && got <= capacity;
}

bool write_file(const char* name, const void* data, size_t size) {
    char full[128]{}; if (!path(full, name)) return false;
    (void)game<uint32_t(*)(const char*,int64_t)>(binding::create_file)(full, int64_t(size));
    uint64_t file = 0;
    last_result = game<uint32_t(*)(uint64_t*,const char*,int32_t)>(binding::open_file)(&file, full, 2);
    if (last_result) return false;
    last_result = game<uint32_t(*)(uint64_t,int64_t)>(binding::set_file_size)(file, int64_t(size));
    if (!last_result) {
        const int32_t flush = 1;
        last_result = game<uint32_t(*)(uint64_t,int64_t,const void*,size_t,const int32_t*)>(binding::write_file)(file, 0, data, size, &flush);
    }
    game<void(*)(uint64_t)>(binding::close_file)(file);
    return last_result == 0;
}

void write_report() {
    const uint32_t operation_result = last_result;
    ScriptRuntimeInfo runtime{}; (void)getScriptRuntimeInfo(&runtime, sizeof(runtime));
    static uint32_t reported_saves = ~0u, reported_loads = ~0u, reported_rejections = ~0u, reported_error = ~0u;
    static uint32_t reported_runtime_resumes = ~0u;
    // A first launch without a settings file can retry loading every frame.
    // Record state changes, not identical missing-file failures on every tick.
    if (reported_saves == save_calls && reported_loads == load_successes &&
        reported_rejections == NativeTable::s_cacheRejected && reported_error == operation_result &&
        reported_runtime_resumes == runtime.resumes) return;
    char report[1536]{}; size_t at = 0;
    auto append = [&](const char* text) { for (size_t i = 0; text[i] && at + 1 < sizeof(report); ++i) report[at++] = text[i]; };
    auto field = [&](const char* name, uint64_t value) {
        append(name); append("=0x"); char digits[16]; unsigned n = 0;
        do { digits[n++] = "0123456789abcdef"[value & 15]; value >>= 4; } while (value);
        while (n && at + 1 < sizeof(report)) report[at++] = digits[--n];
        append("\n");
    };
    append("ScriptHookV-NX cooperative runtime test 1\n");
    field("main_base", Memory::s_mainBase); field("sdk_base", Memory::s_sdkBase);
    field("vm_ticks", NativeTable::s_tickCount); field("cached_native_repairs", NativeTable::s_cacheRepairs);
    field("cached_native_rejections", NativeTable::s_cacheRejected);
    field("save_calls", save_calls); field("load_calls", load_calls);
    field("save_successes", save_successes); field("load_successes", load_successes);
    field("last_fs_result", operation_result);
    field("native_caller_abi", getNativeCallerAbiVersion());
    field("native_caller_self_test_ran", caller_selftest_ran);
    field("native_caller_self_test_passed", caller_selftest.passed);
    field("native_caller_self_test_failed", caller_selftest.failed);
    field("native_caller_self_test_error", caller_selftest.last_error);
    field("native_caller_self_test_error_hash", caller_selftest.last_error_hash);
    field("runtime_abi", runtime.abi); field("runtime_active", runtime.active);
    field("runtime_resumes", runtime.resumes); field("runtime_context_rejections", runtime.context_rejections);
    field("runtime_test_runs", runtime.self_test_runs); field("runtime_test_done", runtime.self_test_done);
    field("runtime_test_passed", runtime.self_test_passed); field("runtime_test_failed", runtime.self_test_failed);
    if (write_file("bridge-report.txt", report, at)) {
        reported_saves = save_calls; reported_loads = load_successes;
        reported_rejections = NativeTable::s_cacheRejected; reported_error = operation_result;
        reported_runtime_resumes = runtime.resumes;
    }
    last_result = operation_result;
}

uint32_t crc(const uint8_t* bytes, size_t count, uint32_t value = ~0u) {
    for (size_t i = 0; i < count; ++i) {
        value ^= bytes[i];
        for (unsigned b = 0; b < 8; ++b) value = (value >> 1) ^ (0xedb88320u & uint32_t(0 - (value & 1)));
    }
    return value;
}
uint32_t checksum(const Record& record) {
    return ~crc(record.data, record.bytes, crc(reinterpret_cast<const uint8_t*>(&record), 16));
}
const char* slot_name(uint32_t storage, unsigned slot) {
    if (storage == 1) return slot ? "storage-test-b.bin" : "storage-test-a.bin";
    if (storage == 2) return slot ? "megatard-vehicles-b.bin" : "megatard-vehicles-a.bin";
    return slot ? "megatard-settings-b.bin" : "megatard-settings-a.bin";
}
bool load_record(const char* name, Record& record) {
    size_t got = 0;
    if (!read_file(name, &record, sizeof(record), got) || got < 20 ||
        record.magic != record_magic || record.version != 1 || !record.generation ||
        !record.bytes || record.bytes > sizeof(record.data) || record.bytes % 4 ||
        got != 20 + record.bytes) return false;
    return record.crc == checksum(record);
}
bool newest(const Record& a, const Record& b) { return int32_t(b.generation - a.generation) > 0; }

bool arguments(void* context, uint64_t*& buffer, size_t& words, uint32_t& storage) {
    const uintptr_t address = reinterpret_cast<uintptr_t>(context);
    if (!Memory::IsReadable(address, sizeof(ScriptContext))) return false;
    auto* call = static_cast<ScriptContext*>(context);
    if (!call->result || !Memory::IsWritable(reinterpret_cast<uintptr_t>(call->result), 8)) return false;
    if (call->count < 2 || call->count > 32 ||
        !Memory::IsReadable(reinterpret_cast<uintptr_t>(call->args), call->count * 8)) return false;
    buffer = reinterpret_cast<uint64_t*>(call->args[0]);
    // Script INTs occupy only the low 32 bits of each eight-byte VM slot.
    // Stock PUSH_CONST instructions use STR W and leave the upper half stale;
    // reading that padding as part of a size/flag rejects valid native calls.
    words = uint32_t(call->args[1]);
    storage = call->count >= 3 ? uint32_t(call->args[2]) : 0;
    const bool valid = words && words <= max_words && storage <= 2;
    // RAGE reuses the first argument slot for the return value. Snapshot all
    // arguments before initializing that slot, or the buffer pointer is lost.
    *static_cast<uint64_t*>(call->result) = 0;
    return valid;
}
void result(void* context, uint64_t value) { *static_cast<uint64_t*>(static_cast<ScriptContext*>(context)->result) = value; }
}

bool Persistence::Initialize() {
    return NativeTable::RegisterCustom(0x4D45474153415645ULL, Native_SaveMenuSettings) &&
           NativeTable::RegisterCustom(0x4D4547414C4F4144ULL, Native_LoadMenuSettings);
}

bool Persistence::Save(const char* relativePath, const void* data, size_t size) {
    if (!size || size > 8192 || !Memory::IsReadable(reinterpret_cast<uintptr_t>(data), size)) return false;
    Session session; return session.mounted && write_file(relativePath, data, size);
}
bool Persistence::Read(const char* relativePath, void* buffer, size_t maxBytes, size_t* bytesRead) {
    if (bytesRead) *bytesRead = 0;
    if (!maxBytes || maxBytes > 8192 || !Memory::IsWritable(reinterpret_cast<uintptr_t>(buffer), maxBytes)) return false;
    Session session; size_t got = 0;
    const bool ok = session.mounted && read_file(relativePath, buffer, maxBytes, got);
    if (bytesRead && ok) *bytesRead = got;
    return ok;
}

void Persistence::Native_SaveMenuSettings(void* ctx) {
    uint64_t* source = nullptr; size_t words = 0; uint32_t storage = 0;
    if (!arguments(ctx, source, words, storage) || !Memory::IsReadable(reinterpret_cast<uintptr_t>(source), words * 8)) return;
    ++save_calls;
    if (storage == 1) {
        caller_selftest_ran = true; (void)nativeCallerSelfTest(&caller_selftest);
        (void)scriptRuntimeSelfTest();
    }
    Session session; if (!session.mounted) return;
    Record a{}, b{}, next{};
    const bool valid_a = load_record(slot_name(storage, 0), a), valid_b = load_record(slot_name(storage, 1), b);
    const Record* chosen = valid_a && valid_b ? (newest(a, b) ? &b : &a) : valid_a ? &a : valid_b ? &b : nullptr;
    const unsigned slot = chosen == &a ? 1u : 0u;
    next.magic = record_magic; next.version = 1; next.generation = chosen ? chosen->generation + 1 : 1;
    if (!next.generation) next.generation = 1;
    next.bytes = uint32_t(words * 4);
    for (size_t i = 0; i < words; ++i) {
        const uint32_t word = uint32_t(source[i]);
        for (unsigned j = 0; j < 4; ++j) next.data[i * 4 + j] = uint8_t(word >> (8 * j));
    }
    next.crc = checksum(next);
    const bool ok = write_file(slot_name(storage, slot), &next, 20 + next.bytes);
    if (ok) ++save_successes;
    result(ctx, ok ? 1 : 0);
}

void Persistence::Native_LoadMenuSettings(void* ctx) {
    uint64_t* destination = nullptr; size_t words = 0; uint32_t storage = 0;
    if (!arguments(ctx, destination, words, storage) || !Memory::IsWritable(reinterpret_cast<uintptr_t>(destination), words * 8)) return;
    ++load_calls;
    Session session; if (!session.mounted) return;
    Record a{}, b{};
    const bool valid_a = load_record(slot_name(storage, 0), a), valid_b = load_record(slot_name(storage, 1), b);
    const Record* chosen = valid_a && valid_b ? (newest(a, b) ? &b : &a) : valid_a ? &a : valid_b ? &b : nullptr;
    if (!chosen || chosen->bytes > words * 4) return;
    const size_t count = chosen->bytes / 4;
    for (size_t i = 0; i < count; ++i) {
        uint32_t word = 0; for (unsigned j = 0; j < 4; ++j) word |= uint32_t(chosen->data[i * 4 + j]) << (8 * j);
        destination[i] = uint64_t(int64_t(int32_t(word)));
    }
    ++load_successes; result(ctx, count);
}
}

extern "C" bool scriptSaveFile(const char* path, const void* data, size_t size) {
    return ScriptHook::Persistence::Save(path, data, size);
}
extern "C" bool scriptReadFile(const char* path, void* data, size_t size, size_t* read) {
    return ScriptHook::Persistence::Read(path, data, size, read);
}
