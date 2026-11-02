#pragma once
#if !defined(SHVNX_PLUGIN_JSON_RUNTIME) || !SHVNX_PLUGIN_JSON_RUNTIME
#error Enable json_runtime and c_runtime for shv/json.h
#endif
#ifdef YYJSON_H
#error Include shv/json.h before the pinned yyjson header
#endif
#include "json_config.h"
#include "../../sdk/third_party/yyjson/yyjson.h"
#include "../plugin_memory.h"
#include "../plugin_resources.h"
#include <new>
#include <utility>

namespace ScriptHook::Json {
constexpr size_t DefaultMemoryLimit = 1024 * 1024;
constexpr size_t MaximumInputBytes = 64 * 1024 * 1024;
enum class ErrorCode : uint32_t { None, InvalidArgument, Unavailable, MemoryLimit, NoMemory, Parse, Write };
struct Error { ErrorCode code = ErrorCode::None; uint32_t library_code = 0; size_t position = 0; };

namespace Detail {
struct Budget {
    size_t limit, used, peak;
    bool refused, failed;
};
inline void* Allocate(void* context, size_t bytes) {
    auto& budget = *static_cast<Budget*>(context);
    if (!bytes || bytes > budget.limit - budget.used) { budget.refused = true; return nullptr; }
    void* result = scriptAllocate(bytes, 16, 0);
    if (!result) { budget.failed = true; return nullptr; }
    budget.used += bytes;
    if (budget.used > budget.peak) budget.peak = budget.used;
    return result;
}
inline void Release(void* context, void* pointer) {
    if (!pointer) return;
    auto& budget = *static_cast<Budget*>(context);
    const size_t bytes = scriptAllocationSize(pointer);
    if (bytes && scriptFree(pointer)) budget.used -= bytes;
    else budget.failed = true;
}
inline void* Reallocate(void* context, void* pointer, size_t, size_t bytes) {
    if (!pointer) return Allocate(context, bytes);
    if (!bytes) { Release(context, pointer); return nullptr; }
    auto& budget = *static_cast<Budget*>(context);
    const size_t old = scriptAllocationSize(pointer);
    if (!old) { budget.failed = true; return nullptr; }
    // Admit the replacement while the old block is still counted. The limit
    // therefore includes transient overlapping buffers rather than final size.
    void* result = Allocate(context, bytes);
    if (!result) return nullptr;
    memcpy(result, pointer, old < bytes ? old : bytes);
    Release(context, pointer);
    return result;
}
struct State {
    Budget budget;
    yyjson_alc allocator;
    yyjson_doc* read;
    yyjson_mut_doc* edit;
    char* output;
    size_t bytes;
};
inline State* Create(size_t limit) {
    if (limit < sizeof(State)) return nullptr;
    void* storage = scriptAllocate(sizeof(State), alignof(State), 0);
    if (!storage) return nullptr;
    auto* state = ::new(storage) State{};
    state->budget = {limit, sizeof(State), sizeof(State), false, false};
    state->allocator = {Allocate, Reallocate, Release, &state->budget};
    return state;
}
inline bool Destroy(State*& state) {
    if (!state) return true;
    if (scriptAllocationSize(state) != sizeof(State) || scriptResourcePinned(state)) return false;
    if (state->read) { yyjson_doc_free(state->read); state->read = nullptr; }
    if (state->edit) { yyjson_mut_doc_free(state->edit); state->edit = nullptr; }
    if (state->output) { Release(&state->budget, state->output); state->output = nullptr; }
    if (state->budget.used != sizeof(State)) return false;
    state->~State();
    if (!scriptFree(state)) return false;
    state = nullptr;
    return true;
}
inline Error Failure(const State* state, ErrorCode fallback, uint32_t code = 0, size_t position = 0) {
    return {state && state->budget.refused ? ErrorCode::MemoryLimit :
            state && state->budget.failed ? ErrorCode::NoMemory : fallback, code, position};
}
}

class Buffer;
class Document final {
    Detail::State* state_ = nullptr;
    Error error_{};
    friend class Buffer;
    bool Publish(Detail::State* next) noexcept {
        if (!Detail::Destroy(state_)) { (void)Detail::Destroy(next); error_ = {ErrorCode::Unavailable}; return false; }
        state_ = next; error_ = {}; return true;
    }
    Detail::State* Candidate(size_t limit) noexcept {
        if (limit < sizeof(Detail::State)) { error_ = {ErrorCode::MemoryLimit}; return nullptr; }
        if (!scriptMemoryAvailable()) { error_ = {ErrorCode::Unavailable}; return nullptr; }
        auto* result = Detail::Create(limit);
        if (!result) error_ = {ErrorCode::NoMemory};
        return result;
    }
    bool Put(const char* key, yyjson_mut_val* value) noexcept {
        if (!state_ || !state_->edit || !key || !yyjson_mut_is_obj(EditRoot())) { error_ = {ErrorCode::InvalidArgument}; return false; }
        auto* name = value ? yyjson_mut_strcpy(state_->edit, key) : nullptr;
        if (!name || !value) { error_ = Detail::Failure(state_, ErrorCode::NoMemory); return false; }
        const bool ok = yyjson_mut_obj_put(EditRoot(), name, value);
        error_ = ok ? Error{} : Error{ErrorCode::InvalidArgument};
        return ok;
    }
    bool Ready(const char* key) noexcept {
        if (!key || !state_ || !state_->edit || !yyjson_mut_is_obj(EditRoot())) { error_ = {ErrorCode::InvalidArgument}; return false; }
        state_->budget.refused = state_->budget.failed = false;
        return true;
    }
public:
    Document() noexcept = default;
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    Document(Document&& other) noexcept : state_(other.state_), error_(other.error_) { other.state_ = nullptr; other.error_ = {}; }
    Document& operator=(Document&& other) noexcept {
        if (this != &other && Reset()) { state_ = other.state_; error_ = other.error_; other.state_ = nullptr; other.error_ = {}; }
        return *this;
    }
    ~Document() noexcept { (void)Reset(); }
    bool Parse(const char* input, size_t bytes, size_t memory_limit = DefaultMemoryLimit) noexcept {
        if (!input || !bytes || bytes > MaximumInputBytes) { error_ = {ErrorCode::InvalidArgument}; return false; }
        auto* next = Candidate(memory_limit);
        if (!next) return false;
        yyjson_read_err detail{};
        next->read = yyjson_read_opts(const_cast<char*>(input), bytes, 0, &next->allocator, &detail);
        if (!next->read) {
            error_ = Detail::Failure(next, ErrorCode::Parse, detail.code, detail.pos);
            (void)Detail::Destroy(next); return false;
        }
        return Publish(next);
    }
    bool CreateObject(size_t memory_limit = DefaultMemoryLimit) noexcept {
        auto* next = Candidate(memory_limit);
        if (!next) return false;
        next->edit = yyjson_mut_doc_new(&next->allocator);
        auto* root = next->edit ? yyjson_mut_obj(next->edit) : nullptr;
        if (!root) {
            error_ = Detail::Failure(next, ErrorCode::NoMemory);
            (void)Detail::Destroy(next); return false;
        }
        yyjson_mut_doc_set_root(next->edit, root);
        return Publish(next);
    }
    bool CopyFrom(const Document& source, size_t memory_limit = DefaultMemoryLimit) noexcept {
        if (this == &source && state_ && state_->edit) { error_ = {}; return true; }
        if (!source.state_) { error_ = {ErrorCode::InvalidArgument}; return false; }
        auto* next = Candidate(memory_limit);
        if (!next) return false;
        next->edit = source.state_->read ? yyjson_doc_mut_copy(source.state_->read, &next->allocator) :
            yyjson_mut_doc_mut_copy(source.state_->edit, &next->allocator);
        if (!next->edit) {
            error_ = Detail::Failure(next, ErrorCode::NoMemory);
            (void)Detail::Destroy(next); return false;
        }
        return Publish(next);
    }
    bool SetUInt(const char* key, uint64_t value) noexcept { return Ready(key) && Put(key, yyjson_mut_uint(state_->edit, value)); }
    bool SetInt(const char* key, int64_t value) noexcept { return Ready(key) && Put(key, yyjson_mut_sint(state_->edit, value)); }
    bool SetBool(const char* key, bool value) noexcept { return Ready(key) && Put(key, yyjson_mut_bool(state_->edit, value)); }
    bool SetNull(const char* key) noexcept { return Ready(key) && Put(key, yyjson_mut_null(state_->edit)); }
    bool SetString(const char* key, const char* value, size_t bytes) noexcept {
        if (!value) { error_ = {ErrorCode::InvalidArgument}; return false; }
        return Ready(key) && Put(key, yyjson_mut_strncpy(state_->edit, value, bytes));
    }
    bool SetReal(const char* key, double value) noexcept {
        uint64_t bits = 0; memcpy(&bits, &value, sizeof(bits));
        if ((bits & UINT64_C(0x7ff0000000000000)) == UINT64_C(0x7ff0000000000000)) { error_ = {ErrorCode::InvalidArgument}; return false; }
        return Ready(key) && Put(key, yyjson_mut_real(state_->edit, value));
    }
    const yyjson_val* ReadRoot() const noexcept { return state_ && state_->read ? yyjson_doc_get_root(state_->read) : nullptr; }
    yyjson_mut_val* EditRoot() noexcept { return state_ && state_->edit ? yyjson_mut_doc_get_root(state_->edit) : nullptr; }
    yyjson_mut_doc* NativeMutable() noexcept { return state_ ? state_->edit : nullptr; }
    size_t MemoryBytes() const noexcept { return state_ ? state_->budget.used : 0; }
    size_t PeakBytes() const noexcept { return state_ ? state_->budget.peak : 0; }
    Error LastError() const noexcept { return error_; }
    bool Reset() noexcept { if (!Detail::Destroy(state_)) return false; error_ = {}; return true; }
};

class Buffer final {
    Detail::State* state_ = nullptr;
    Error error_{};
public:
    Buffer() noexcept = default;
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept : state_(other.state_), error_(other.error_) { other.state_ = nullptr; other.error_ = {}; }
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other && Reset()) { state_ = other.state_; error_ = other.error_; other.state_ = nullptr; other.error_ = {}; }
        return *this;
    }
    ~Buffer() noexcept { (void)Reset(); }
    bool Serialize(const Document& source, size_t maximum_bytes, size_t memory_limit = DefaultMemoryLimit) noexcept {
        if (!source.state_) { error_ = {ErrorCode::InvalidArgument}; return false; }
        if (memory_limit < sizeof(Detail::State)) { error_ = {ErrorCode::MemoryLimit}; return false; }
        auto* next = Detail::Create(memory_limit);
        if (!next) { error_ = {ErrorCode::NoMemory}; return false; }
        yyjson_write_err detail{};
        next->output = source.state_->read ? yyjson_write_opts(source.state_->read, 0, &next->allocator, &next->bytes, &detail) :
            yyjson_mut_write_opts(source.state_->edit, 0, &next->allocator, &next->bytes, &detail);
        if (!next->output || next->bytes > maximum_bytes) {
            error_ = Detail::Failure(next, next->output ? ErrorCode::MemoryLimit : ErrorCode::Write, detail.code);
            (void)Detail::Destroy(next); return false;
        }
        if (!Detail::Destroy(state_)) { (void)Detail::Destroy(next); error_ = {ErrorCode::Unavailable}; return false; }
        state_ = next; error_ = {}; return true;
    }
    const char* Text() const noexcept { return state_ ? state_->output : nullptr; }
    size_t Bytes() const noexcept { return state_ ? state_->bytes : 0; }
    size_t MemoryBytes() const noexcept { return state_ ? state_->budget.used : 0; }
    Error LastError() const noexcept { return error_; }
    bool Reset() noexcept { if (!Detail::Destroy(state_)) return false; error_ = {}; return true; }
};
}
