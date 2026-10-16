#pragma once
#include "natives.h"

namespace ScriptHook { namespace Util {
// Length of one valid UTF-8 scalar; zero rejects incomplete/invalid input.
inline size_t Utf8Unit(const char* text) {
    const auto* p = reinterpret_cast<const uint8_t*>(text);
    if (!p || !p[0]) return 0;
    if (p[0] < 0x80) return 1;
    const size_t n = p[0] >= 0xc2 && p[0] <= 0xdf ? 2 :
                     p[0] >= 0xe0 && p[0] <= 0xef ? 3 : p[0] >= 0xf0 && p[0] <= 0xf4 ? 4 : 0;
    if (!n) return 0;
    for (size_t i = 1; i < n; ++i) if (!p[i] || (p[i] & 0xc0) != 0x80) return 0;
    if ((p[0] == 0xe0 && p[1] < 0xa0) || (p[0] == 0xed && p[1] >= 0xa0) ||
        (p[0] == 0xf0 && p[1] < 0x90) || (p[0] == 0xf4 && p[1] >= 0x90)) return 0;
    return n;
}
template<size_t Capacity = 192> class TextBuffer {
    static_assert(Capacity >= 4, "Text buffer needs at least four bytes");
    char value[Capacity]{};
    size_t used = 0;
    bool complete = true;
public:
    TextBuffer& Add(const char* text) {
        if (!text) { complete = false; return *this; }
        while (*text) {
            const size_t n = Utf8Unit(text);
            if (!n || used + n >= Capacity) { complete = false; break; }
            for (size_t i = 0; i < n; ++i) value[used++] = *text++;
        }
        value[used] = 0; return *this;
    }
    TextBuffer& UInt(uint32_t number) {
        char reversed[10], digits[11]{}; size_t n = 0, out = 0;
        do { reversed[n++] = char('0' + number % 10); number /= 10; } while (number);
        while (n) digits[out++] = reversed[--n];
        return Add(digits);
    }
    TextBuffer& Int(int32_t number) {
        if (number < 0) Add("-");
        return UInt(number < 0 ? 0u - uint32_t(number) : uint32_t(number));
    }
    const char* CStr() const { return value; }
    size_t Size() const { return used; }
    bool Complete() const { return complete; }
};
// Use between matching BEGIN/END_TEXT_COMMAND calls. Target limit: ten
// components, each holding 99 UTF-8 bytes. Returns false for overflow/invalid
// text; a valid prefix may already be submitted. Keep HUD formatting tokens
// out of input: this helper handles plain text, not token-aware rich text.
inline bool AppendHudText(const char* text) {
    if (!text) return false;
    for (unsigned component = 0; component < 10 && *text; ++component) {
        char piece[100]{}; size_t used = 0;
        while (*text) {
            const size_t n = Utf8Unit(text);
            if (!n) return false;
            if (used + n > 99) break;
            for (size_t i = 0; i < n; ++i) piece[used++] = *text++;
        }
        Natives::HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(piece);
    }
    return !*text;
}
} }
