#pragma once
#include "json.h"
#include "file_transfer.h"

namespace ScriptHook::Json {
enum class LoadState : uint32_t { Idle, Reading, Parsing, Complete, Failed, Cancelled };

// Owns the transfer buffer and last successfully parsed document. Each Step
// performs one bounded file operation or one synchronous parse, without WAIT.
class FileLoadTask final {
    Data::FileReadTask reader_;
    Document document_;
    size_t memory_limit_ = DefaultMemoryLimit;
    uint64_t transferred_ = 0;
    LoadState state_ = LoadState::Idle;
    Error error_{};
public:
    FileLoadTask() noexcept = default;
    FileLoadTask(const FileLoadTask&) = delete;
    FileLoadTask& operator=(const FileLoadTask&) = delete;
    bool Begin(const char* path, uint64_t maximum_bytes, size_t memory_limit = DefaultMemoryLimit, size_t chunk = 16384) noexcept {
        if (state_ == LoadState::Reading || state_ == LoadState::Parsing) return false;
        if (!reader_.Reset()) return false;
        error_ = {};
        transferred_ = 0;
        memory_limit_ = memory_limit;
        if (!reader_.Begin(path, maximum_bytes, chunk)) { state_ = LoadState::Failed; return false; }
        state_ = LoadState::Reading;
        return true;
    }
    LoadState Step() noexcept {
        if (state_ == LoadState::Reading) {
            const auto result = reader_.Step();
            transferred_ = reader_.Transferred();
            if (result == Data::TransferState::Complete) state_ = LoadState::Parsing;
            else if (result == Data::TransferState::Failed) state_ = LoadState::Failed;
        } else if (state_ == LoadState::Parsing) {
            const bool ok = document_.Parse(reader_.Text(), size_t(reader_.Size()), memory_limit_);
            error_ = document_.LastError();
            if (!reader_.Reset()) { state_ = LoadState::Failed; return state_; }
            state_ = ok ? LoadState::Complete : LoadState::Failed;
        }
        return state_;
    }
    bool Cancel() noexcept {
        if (!reader_.Cancel()) return false;
        state_ = LoadState::Cancelled;
        return true;
    }
    bool Reset() noexcept {
        if (!reader_.Reset() || !document_.Reset()) return false;
        state_ = LoadState::Idle; error_ = {}; transferred_ = 0;
        return true;
    }
    const Document& Current() const noexcept { return document_; }
    LoadState State() const noexcept { return state_; }
    Error LastError() const noexcept { return error_; }
    Data::TransferError TransferError() const noexcept { return reader_.Error(); }
    ShvNxFileStatus FileStatus() const noexcept { return reader_.FileStatus(); }
    uint64_t Transferred() const noexcept { return transferred_; }
};
}
