#pragma once
#include "xml.h"

namespace ScriptHook::Xml {

enum class BufferError : uint32_t { None, Unavailable, InvalidLimit, LimitExceeded, NoMemory };

// Fallible, owner-backed serialization. Output remains private until the entire
// document was written successfully. Serialization itself does not yield.
class OwnedBufferWriter final : public pugi::xml_writer {
public:
    OwnedBufferWriter() noexcept = default;
    ~OwnedBufferWriter() noexcept override { (void)Reset(); }
    OwnedBufferWriter(const OwnedBufferWriter&) = delete;
    OwnedBufferWriter& operator=(const OwnedBufferWriter&) = delete;

    bool Serialize(const pugi::xml_document& document, size_t maximum) noexcept {
        if (writing_) return false;
        complete_ = false; used_ = 0; error_ = BufferError::None;
        if (maximum == SIZE_MAX) { error_ = BufferError::InvalidLimit; return false; }
        if (!scriptMemoryAvailable()) { error_ = BufferError::Unavailable; return false; }
        maximum_ = maximum;
        writing_ = true;
        document.save(*this,"",pugi::format_raw,pugi::encoding_utf8);
        writing_ = false;
        if (error_ != BufferError::None) return false;
        if (!Reserve(used_+1)) return false;
        data_[used_] = 0;
        complete_ = true;
        return true;
    }
    bool Reset() noexcept {
        if (writing_ || (data_ && !scriptFree(data_))) return false;
        data_ = nullptr; used_ = capacity_ = maximum_ = 0;
        complete_ = false; error_ = BufferError::None;
        return true;
    }
    const char* Text() const noexcept { return complete_ ? data_ : nullptr; }
    const void* Data() const noexcept { return Text(); }
    size_t Bytes() const noexcept { return complete_ ? used_ : 0; }
    size_t Capacity() const noexcept { return capacity_; }
    bool Complete() const noexcept { return complete_; }
    BufferError Error() const noexcept { return error_; }
    static void operator delete(void* pointer) noexcept { (void)scriptFree(pointer); }

private:
    char* data_ = nullptr;
    size_t used_ = 0, capacity_ = 0, maximum_ = 0;
    bool complete_ = false, writing_ = false;
    BufferError error_ = BufferError::None;

    bool Reserve(size_t needed) noexcept {
        if (needed <= capacity_) return true;
        size_t next = capacity_ ? capacity_ : 128;
        const size_t ceiling = maximum_+1;
        while (next < needed) {
            if (next > ceiling/2) { next = ceiling; break; }
            next *= 2;
        }
        if (next > ceiling) next = ceiling;
        auto* replacement = static_cast<char*>(data_ ? scriptReallocate(data_,next) : scriptAllocate(next,16,0));
        if (!replacement) { error_ = BufferError::NoMemory; return false; }
        data_ = replacement; capacity_ = next;
        return true;
    }
    void write(const void* input, size_t bytes) override {
        if (!writing_ || error_ != BufferError::None) return;
        if (bytes > maximum_-used_) { error_ = BufferError::LimitExceeded; return; }
        if (!Reserve(used_+bytes+1)) return;
        if (bytes) memcpy(data_+used_,input,bytes);
        used_ += bytes;
        data_[used_] = 0;
    }
};
}
