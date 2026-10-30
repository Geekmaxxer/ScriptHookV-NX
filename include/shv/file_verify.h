#pragma once
#include "file_transfer.h"

namespace ScriptHook::Data {

// Compare a file against independently supplied spans without retaining their
// pointers. Only the bounded read scratch buffer belongs to this task.
class FileVerifyTask final {
public:
    FileVerifyTask() noexcept = default;
    ~FileVerifyTask() noexcept { (void)Reset(); }
    FileVerifyTask(const FileVerifyTask&) = delete;
    FileVerifyTask& operator=(const FileVerifyTask&) = delete;

    bool Begin(const char* path, uint64_t bytes, size_t chunk = 16384) noexcept {
        if (state_ != TransferState::Idle) return false;
        if (!TransferDetail::CopyPath(path,path_) || bytes > SHV_NX_FILE_MAX_BYTES ||
                !chunk || chunk > SHV_NX_FILE_MAX_CHUNK)
            return Fail(TransferError::InvalidArgument,ShvNxFile_InvalidArgument);
        if (!scriptFilesAvailable() || !scriptMemoryAvailable())
            return Fail(TransferError::Unavailable,ShvNxFile_Unavailable);
        uint64_t actual = 0;
        const auto status = scriptFileSize(path_,&actual);
        if (status != ShvNxFile_Ok) return Fail(TransferError::FileError,status);
        if (actual != bytes) return Fail(TransferError::SizeChanged,ShvNxFile_VerificationFailed);
        bytes_ = bytes; chunk_ = chunk;
        const size_t scratch = bytes < chunk ? size_t(bytes) : chunk;
        if (scratch) {
            buffer_ = static_cast<uint8_t*>(scriptAllocate(scratch,16,0));
            if (!buffer_) return Fail(TransferError::AllocationFailed,ShvNxFile_Ok);
        }
        state_ = TransferState::Active;
        return true;
    }
    TransferState Compare(const void* expected, size_t bytes) noexcept {
        if (state_ != TransferState::Active) return state_;
        if (!expected || !bytes || bytes > chunk_ || bytes > bytes_-offset_)
            (void)Fail(TransferError::InvalidArgument,ShvNxFile_InvalidArgument);
        else {
            size_t received = 0;
            const auto status = scriptFileReadAt(path_,offset_,buffer_,bytes,&received);
            if (status != ShvNxFile_Ok) (void)Fail(TransferError::FileError,status);
            else if (received != bytes) (void)Fail(TransferError::Incomplete,ShvNxFile_IncompleteRead);
            else {
                const auto* reference = static_cast<const uint8_t*>(expected);
                for (size_t i = 0; i < bytes; ++i) {
                    if (buffer_[i] != reference[i]) {
                        (void)Fail(TransferError::ContentMismatch,ShvNxFile_VerificationFailed);
                        break;
                    }
                }
                if (state_ == TransferState::Active) offset_ += bytes;
            }
        }
        return state_;
    }
    TransferState Finish() noexcept {
        if (state_ != TransferState::Active) return state_;
        if (offset_ != bytes_) (void)Fail(TransferError::Incomplete,ShvNxFile_Ok);
        else {
            uint64_t actual = 0;
            const auto status = scriptFileSize(path_,&actual);
            if (status != ShvNxFile_Ok) (void)Fail(TransferError::FileError,status);
            else if (actual != bytes_) (void)Fail(TransferError::SizeChanged,ShvNxFile_VerificationFailed);
            else state_ = TransferState::Complete;
        }
        return state_;
    }
    bool Reset() noexcept {
        if (buffer_ && !scriptFree(buffer_)) return false;
        buffer_ = nullptr; bytes_ = offset_ = chunk_ = 0; path_[0] = 0;
        state_ = TransferState::Idle; error_ = TransferError::None; file_status_ = ShvNxFile_Ok;
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
}
