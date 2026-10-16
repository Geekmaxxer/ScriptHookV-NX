#pragma once
#include "plugin.h"

namespace ScriptHook { namespace Util {
inline uint32_t Crc32(const void* data, size_t size) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    uint32_t crc = ~0u;
    for (size_t i = 0; i < size; ++i) {
        crc ^= bytes[i];
        for (unsigned j = 0; j < 8; ++j) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

// One fixed, versioned array of uint32_t settings per plugin. No heap, globals
// with dynamic initialization, automatic polling, or writes from destructors.
// Call Load once; false leaves caller defaults intact. Save also probes slots
// if Load was omitted. Override filenames when storing more than one record.
template<size_t Words> class SavedSettings {
    static_assert(Words > 0 && Words <= 256, "Settings require 1-256 uint32_t words");
    struct Record { uint32_t tag, schema, generation, count, words[Words], crc; };
    static_assert(sizeof(Record) == (Words + 5) * sizeof(uint32_t));
    uint32_t tag, schema, generation = 0;
    unsigned selected = 1;
    bool probed = false;
    const char* names[2];
    bool Read(unsigned slot, Record& record) const {
        size_t got = 0;
        return scriptReadFile(names[slot], &record, sizeof(record), &got) && got == sizeof(record) &&
            record.tag == tag && record.schema == schema && record.count == Words &&
            record.crc == Crc32(&record, offsetof(Record, crc));
    }
public:
    explicit SavedSettings(uint32_t formatTag, uint32_t version = 1,
                           const char* slotA = "settings-a.bin", const char* slotB = "settings-b.bin")
        : tag(formatTag), schema(version), names{slotA, slotB} {}
    bool Load(uint32_t (&words)[Words]) {
        Record a{}, b{};
        const bool validA = Read(0, a), validB = Read(1, b);
        probed = true; selected = 1; generation = 0;
        if (!validA && !validB) return false;
        const uint32_t difference = b.generation - a.generation;
        selected = validB && (!validA || (difference && difference < 0x80000000u)) ? 1 : 0;
        const Record& current = selected ? b : a;
        generation = current.generation;
        for (size_t i = 0; i < Words; ++i) words[i] = current.words[i];
        return true;
    }
    bool Save(const uint32_t (&words)[Words]) {
        if (!probed) { uint32_t ignored[Words]{}; (void)Load(ignored); }
        Record record{};
        record.tag = tag; record.schema = schema; record.generation = generation + 1; record.count = Words;
        for (size_t i = 0; i < Words; ++i) record.words[i] = words[i];
        record.crc = Crc32(&record, offsetof(Record, crc));
        const unsigned next = selected ^ 1u;
        if (!scriptSaveFile(names[next], &record, sizeof(record))) return false;
        Record check{};
        if (!Read(next, check) || check.generation != record.generation) return false;
        for (size_t i = 0; i < Words; ++i) if (check.words[i] != record.words[i]) return false;
        generation = record.generation; selected = next; return true;
    }
};
} }
