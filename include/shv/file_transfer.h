#pragma once
#include "../plugin_files.h"
#include "../plugin_memory.h"

namespace ScriptHook::Data {

enum class TransferState : uint32_t { Idle, Active, Complete, Failed, Cancelled };
enum class TransferError : uint32_t {
    None, InvalidArgument, Unavailable, LimitExceeded, AllocationFailed,
    FileError, SizeChanged, Incomplete, ContentMismatch
};

namespace TransferDetail {
inline bool CopyPath(const char* input, char (&output)[SHV_NX_FILE_MAX_PATH+1]) noexcept {
    if (!input) return false;
    for (size_t i = 0; i <= SHV_NX_FILE_MAX_PATH; ++i) {
        const char value = input[i];
        if (!value) { output[i] = 0; return i != 0; }
        if (i == SHV_NX_FILE_MAX_PATH) return false;
        output[i] = value;
    }
    return false;
}
}

// Each Step performs one bounded file operation. No WAIT or file handle is
// retained internally. Own this object through Managed/SessionLocal across WAIT.
class FileReadTask final {
public:
    FileReadTask() noexcept = default;
    ~FileReadTask() noexcept { (void)Reset(); }
    FileReadTask(const FileReadTask&) = delete;
    FileReadTask& operator=(const FileReadTask&) = delete;

    bool Begin(const char* path, uint64_t maximum, size_t chunk = 16384) noexcept {
        if (state_ != TransferState::Idle) return false;
        if (!TransferDetail::CopyPath(path,path_) || maximum > SHV_NX_FILE_MAX_BYTES ||
                !chunk || chunk > SHV_NX_FILE_MAX_CHUNK)
            return Fail(TransferError::InvalidArgument,ShvNxFile_InvalidArgument);
        if (!scriptFilesAvailable() || !scriptMemoryAvailable())
            return Fail(TransferError::Unavailable,ShvNxFile_Unavailable);
        const auto status = scriptFileSize(path_,&bytes_);
        if (status != ShvNxFile_Ok) return Fail(TransferError::FileError,status);
        if (bytes_ > maximum || bytes_ >= SIZE_MAX)
            return Fail(TransferError::LimitExceeded,ShvNxFile_OutOfRange);
        buffer_ = static_cast<uint8_t*>(scriptAllocate(size_t(bytes_)+1,16,0));
        if (!buffer_) return Fail(TransferError::AllocationFailed,ShvNxFile_Ok);
        buffer_[bytes_] = 0;
        chunk_ = chunk;
        state_ = TransferState::Active;
        return true;
    }

    TransferState Step() noexcept {
        if (state_ != TransferState::Active) return state_;
        if (offset_ < bytes_) {
            const size_t wanted = bytes_-offset_ < chunk_ ? size_t(bytes_-offset_) : chunk_;
            size_t received = 0;
            const auto status = scriptFileReadAt(path_,offset_,buffer_+offset_,wanted,&received);
            if (status != ShvNxFile_Ok) (void)Fail(TransferError::FileError,status);
            else if (received != wanted) (void)Fail(TransferError::Incomplete,ShvNxFile_IncompleteRead);
            else offset_ += received;
        } else {
            uint64_t current = 0;
            const auto status = scriptFileSize(path_,&current);
            if (status != ShvNxFile_Ok) (void)Fail(TransferError::FileError,status);
            else if (current != bytes_) (void)Fail(TransferError::SizeChanged,ShvNxFile_VerificationFailed);
            else state_ = TransferState::Complete;
        }
        return state_;
    }

    bool Reset() noexcept {
        if (buffer_ && !scriptFree(buffer_)) return false;
        buffer_ = nullptr;
        bytes_ = offset_ = chunk_ = 0;
        state_ = TransferState::Idle;
        error_ = TransferError::None;
        file_status_ = ShvNxFile_Ok;
        path_[0] = 0;
        return true;
    }
    bool Cancel() noexcept {
        if (!Reset()) return false;
        state_ = TransferState::Cancelled;
        return true;
    }
    TransferState State() const noexcept { return state_; }
    TransferError Error() const noexcept { return error_; }
    ShvNxFileStatus FileStatus() const noexcept { return file_status_; }
    uint64_t Size() const noexcept { return bytes_; }
    uint64_t Transferred() const noexcept { return offset_; }
    const uint8_t* Data() const noexcept { return state_ == TransferState::Complete ? buffer_ : nullptr; }
    const char* Text() const noexcept { return reinterpret_cast<const char*>(Data()); }

private:
    char path_[SHV_NX_FILE_MAX_PATH+1]{};
    uint8_t* buffer_ = nullptr;
    uint64_t bytes_ = 0, offset_ = 0;
    size_t chunk_ = 0;
    TransferState state_ = TransferState::Idle;
    TransferError error_ = TransferError::None;
    ShvNxFileStatus file_status_ = ShvNxFile_Ok;
    bool Fail(TransferError error, ShvNxFileStatus status) noexcept {
        state_ = TransferState::Failed; error_ = error; file_status_ = status;
        return false;
    }
};

// Create a new prepared file exclusively and supply one temporary input span per
// Append. No input pointer survives the call; existing destinations are preserved.
class PreparedFileWriteTask final {
public:
    PreparedFileWriteTask() noexcept = default;
    PreparedFileWriteTask(const PreparedFileWriteTask&) = delete;
    PreparedFileWriteTask& operator=(const PreparedFileWriteTask&) = delete;

    bool Begin(const char* path, uint64_t bytes) noexcept {
        if (state_ != TransferState::Idle) return false;
        if (!TransferDetail::CopyPath(path,path_) || bytes > SHV_NX_FILE_MAX_BYTES)
            return Fail(TransferError::InvalidArgument,ShvNxFile_InvalidArgument);
        if (!scriptFilesAvailable()) return Fail(TransferError::Unavailable,ShvNxFile_Unavailable);
        const auto status = scriptFileCreate(path_,bytes);
        if (status != ShvNxFile_Ok) return Fail(TransferError::FileError,status);
        bytes_ = bytes;
        state_ = TransferState::Active;
        return true;
    }
    TransferState Append(const void* data, size_t bytes) noexcept {
        if (state_ != TransferState::Active) return state_;
        if (!data || !bytes || bytes > SHV_NX_FILE_MAX_CHUNK || bytes > bytes_-offset_)
            (void)Fail(TransferError::InvalidArgument,ShvNxFile_InvalidArgument);
        else {
            const auto status = scriptFileWriteAt(path_,offset_,data,bytes);
            if (status != ShvNxFile_Ok) (void)Fail(TransferError::FileError,status);
            else offset_ += bytes;
        }
        return state_;
    }
    TransferState Finish() noexcept {
        if (state_ != TransferState::Active) return state_;
        if (offset_ != bytes_) (void)Fail(TransferError::Incomplete,ShvNxFile_Ok);
        else {
            uint64_t current = 0;
            const auto status = scriptFileSize(path_,&current);
            if (status != ShvNxFile_Ok) (void)Fail(TransferError::FileError,status);
            else if (current != bytes_) (void)Fail(TransferError::SizeChanged,ShvNxFile_VerificationFailed);
            else state_ = TransferState::Complete;
        }
        return state_;
    }
    void Reset() noexcept {
        bytes_ = offset_ = 0; path_[0] = 0;
        state_ = TransferState::Idle; error_ = TransferError::None; file_status_ = ShvNxFile_Ok;
    }
    void Cancel() noexcept { state_ = TransferState::Cancelled; }
    TransferState State() const noexcept { return state_; }
    TransferError Error() const noexcept { return error_; }
    ShvNxFileStatus FileStatus() const noexcept { return file_status_; }
    uint64_t Size() const noexcept { return bytes_; }
    uint64_t Transferred() const noexcept { return offset_; }

private:
    char path_[SHV_NX_FILE_MAX_PATH+1]{};
    uint64_t bytes_ = 0, offset_ = 0;
    TransferState state_ = TransferState::Idle;
    TransferError error_ = TransferError::None;
    ShvNxFileStatus file_status_ = ShvNxFile_Ok;
    bool Fail(TransferError error, ShvNxFileStatus status) noexcept {
        state_ = TransferState::Failed; error_ = error; file_status_ = status;
        return false;
    }
};
}
