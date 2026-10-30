#pragma once
#include "../plugin_memory.h"

namespace ScriptHook::Data {

enum class BlobStatus : uint32_t {
    Ok, InvalidArgument, NoMemory, NoValidRecord, ReadFailed, WriteFailed,
    VerificationFailed, BufferTooSmall, Conflict, AlreadyExists
};

// Two checked records in the plugin's isolated settings directory. Operations
// are synchronous and owner-thread only; no filesystem signatures are added.
class BlobStore {
public:
    static constexpr size_t HeaderBytes = 32;
    static constexpr size_t MaximumPayload = 65536 - HeaderBytes;

    explicit BlobStore(uint32_t schema, const char* slotA = "data-a.bin",
                       const char* slotB = "data-b.bin") noexcept : schema_(schema) {
        valid_ = Name(slotA, names_[0]) && Name(slotB, names_[1]);
        bool equal = true;
        for (size_t i = 0; i < 64; ++i) {
            char a = names_[0][i], b = names_[1][i];
            if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
            if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
            if (a != b) { equal = false; break; }
            if (!a) break;
        }
        valid_ = valid_ && !equal;
    }

    // bytes/generation are zeroed on failure; payload output is unchanged.
    BlobStatus Load(void* output, size_t capacity, size_t& bytes,
                    uint64_t* generation = nullptr) const noexcept {
        bytes = 0;
        if (generation) *generation = 0;
        if (!valid_ || (!output && capacity)) return BlobStatus::InvalidArgument;
        Scratch scratch;
        if (!scratch.data) return BlobStatus::NoMemory;
        Record records[2]; unsigned selected = 0;
        const auto status = Scan(scratch.data, records, selected);
        if (status != BlobStatus::Ok) return status;
        const Record expected = records[selected];
        if (capacity < expected.bytes) return BlobStatus::BufferTooSmall;
        const Record current = Read(selected, scratch.data);
        if (!current.read) return BlobStatus::ReadFailed;
        if (!Same(expected, current)) return BlobStatus::VerificationFailed;
        auto* out = static_cast<uint8_t*>(output);
        for (size_t i = 0; i < current.bytes; ++i) out[i] = scratch.data[HeaderBytes + i];
        bytes = current.bytes;
        if (generation) *generation = current.generation;
        return BlobStatus::Ok;
    }

    // Normal Save refuses an unreadable slot. The bool file API cannot prove
    // whether it is missing or temporarily inaccessible. Initialize/Repair are
    // deliberate recovery operations, never implicit fallbacks from Save.
    BlobStatus Save(const void* payload, size_t bytes) const noexcept {
        return Update(payload, bytes, false);
    }

    BlobStatus Repair(const void* payload, size_t bytes) const noexcept {
        return Update(payload, bytes, true);
    }

    // Explicit authorization to create/replace invalid records when neither
    // slot validates. A valid existing slot always prevents initialization.
    BlobStatus Initialize(const void* payload, size_t bytes) const noexcept {
        if (!Input(payload, bytes)) return BlobStatus::InvalidArgument;
        Scratch scratch;
        if (!scratch.data) return BlobStatus::NoMemory;
        Record records[2]; unsigned selected = 0;
        const auto status = Scan(scratch.data, records, selected);
        if (status == BlobStatus::Ok) return BlobStatus::AlreadyExists;
        if (status != BlobStatus::NoValidRecord) return status;
        auto written = Write(0, 0, payload, bytes, scratch.data);
        if (written != BlobStatus::Ok) return written;
        return Write(1, 1, payload, bytes, scratch.data);
    }

private:
    struct Scratch {
        uint8_t* data = static_cast<uint8_t*>(scriptAllocate(65536, 16, 0));
        ~Scratch() noexcept { if (data) (void)scriptFree(data); }
        Scratch() noexcept = default;
        Scratch(const Scratch&) = delete;
        Scratch& operator=(const Scratch&) = delete;
    };
    struct Record {
        bool read{}, valid{};
        uint64_t generation{};
        uint32_t bytes{}, crc{};
    };
    uint32_t schema_;
    char names_[2][64]{};
    bool valid_{};

    static bool Name(const char* input, char (&out)[64]) noexcept {
        if (!input || !*input || *input == '.') return false;
        for (size_t i = 0; i < 64; ++i) {
            const char c = input[i];
            if (!c) { out[i] = 0; return i && input[i - 1] != '.'; }
            if (i == 63 || (c == '.' && i && input[i - 1] == '.')) return false;
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
            out[i] = c;
        }
        return false;
    }
    bool Input(const void* payload, size_t bytes) const noexcept {
        return valid_ && bytes <= MaximumPayload && (payload || !bytes);
    }
    static uint32_t Word(const uint8_t* data, size_t offset) noexcept {
        uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i) value |= uint32_t(data[offset + i]) << (i * 8);
        return value;
    }
    static void Word(uint8_t* data, size_t offset, uint32_t value) noexcept {
        for (unsigned i = 0; i < 4; ++i) data[offset + i] = uint8_t(value >> (i * 8));
    }
    static uint32_t Checksum(const uint8_t* data, size_t bytes) noexcept {
        uint32_t crc = ~0u;
        for (size_t i = 0; i < bytes; ++i) {
            if (i >= 28 && i < HeaderBytes) continue;
            crc ^= data[i];
            for (unsigned bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
        }
        return ~crc;
    }
    Record Read(unsigned slot, uint8_t* data) const noexcept {
        Record record; size_t bytes = 0;
        record.read = scriptReadFile(names_[slot], data, 65536, &bytes);
        if (!record.read || bytes < HeaderBytes || Word(data, 0) != 0x3142584e ||
            Word(data, 4) != 1 || Word(data, 8) != schema_ || Word(data, 24) != 0) return record;
        record.bytes = Word(data, 20);
        if (record.bytes > MaximumPayload || bytes != HeaderBytes + record.bytes) return record;
        record.generation = uint64_t(Word(data, 12)) | (uint64_t(Word(data, 16)) << 32);
        record.crc = Word(data, 28);
        record.valid = record.crc == Checksum(data, bytes);
        return record;
    }
    static bool Same(const Record& a, const Record& b) noexcept {
        return a.valid && b.valid && a.generation == b.generation &&
               a.bytes == b.bytes && a.crc == b.crc;
    }
    BlobStatus Scan(uint8_t* data, Record (&records)[2], unsigned& selected) const noexcept {
        records[0] = Read(0, data); records[1] = Read(1, data);
        if (!records[0].valid && !records[1].valid) return BlobStatus::NoValidRecord;
        selected = records[1].valid ? 1 : 0;
        if (records[0].valid && records[1].valid) {
            const uint64_t difference = records[1].generation - records[0].generation;
            if (difference == (uint64_t(1) << 63) ||
                (!difference && !Same(records[0], records[1]))) return BlobStatus::Conflict;
            selected = difference && difference < (uint64_t(1) << 63) ? 1 : 0;
        }
        return BlobStatus::Ok;
    }
    BlobStatus Update(const void* payload, size_t bytes, bool recovery) const noexcept {
        if (!Input(payload, bytes)) return BlobStatus::InvalidArgument;
        Scratch scratch;
        if (!scratch.data) return BlobStatus::NoMemory;
        Record records[2]; unsigned selected = 0;
        const auto status = Scan(scratch.data, records, selected);
        if (status != BlobStatus::Ok) return status;
        if (!recovery && (!records[0].read || !records[1].read)) return BlobStatus::ReadFailed;
        return Write(selected ^ 1u, records[selected].generation + 1, payload, bytes, scratch.data);
    }
    BlobStatus Write(unsigned slot, uint64_t generation, const void* payload,
                     size_t bytes, uint8_t* data) const noexcept {
        Word(data, 0, 0x3142584e); Word(data, 4, 1); Word(data, 8, schema_);
        Word(data, 12, uint32_t(generation)); Word(data, 16, uint32_t(generation >> 32));
        Word(data, 20, uint32_t(bytes)); Word(data, 24, 0); Word(data, 28, 0);
        const auto* input = static_cast<const uint8_t*>(payload);
        for (size_t i = 0; i < bytes; ++i) data[HeaderBytes + i] = input[i];
        const uint32_t crc = Checksum(data, HeaderBytes + bytes); Word(data, 28, crc);
        if (!scriptSaveFile(names_[slot], data, HeaderBytes + bytes)) return BlobStatus::WriteFailed;
        const Record current = Read(slot, data);
        if (!current.valid || current.generation != generation || current.bytes != bytes || current.crc != crc)
            return BlobStatus::VerificationFailed;
        for (size_t i = 0; i < bytes; ++i)
            if (data[HeaderBytes + i] != input[i]) return BlobStatus::VerificationFailed;
        return BlobStatus::Ok;
    }
};
}
