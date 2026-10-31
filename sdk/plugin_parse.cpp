// C-profile integer parsing and error state. No allocation, locale or streams.
#include "shv/crt.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>

namespace {
static_assert(sizeof(long long) == 8,"The parser requires the AArch64 64-bit long-long ABI");
int error_value;

bool Space(char value) noexcept {
    return value == ' ' || value == '\t' || value == '\n' || value == '\r' ||
        value == '\f' || value == '\v';
}

unsigned Digit(char value) noexcept {
    if (value >= '0' && value <= '9') return unsigned(value-'0');
    if (value >= 'a' && value <= 'z') return unsigned(value-'a')+10;
    if (value >= 'A' && value <= 'Z') return unsigned(value-'A')+10;
    return 36;
}

struct ParsedInteger {
    uint64_t magnitude = 0;
    bool negative = false;
    bool overflow = false;
};

ParsedInteger Parse(const char* input,char** end,int base,uint64_t positive_limit,
        uint64_t negative_limit) noexcept {
    ParsedInteger result;
    if (end) *end = const_cast<char*>(input);
    if (base && (base < 2 || base > 36)) { errno = EINVAL; return result; }
    const char* text = input;
    while (Space(*text)) ++text;
    if (*text == '-' || *text == '+') result.negative = *text++ == '-';
    if ((!base || base == 16) && text[0] == '0' && (text[1] == 'x' || text[1] == 'X') && Digit(text[2]) < 16) {
        base = 16; text += 2;
    }
    if (!base) base = *text == '0' ? 8 : 10;
    const uint64_t limit = result.negative ? negative_limit : positive_limit;
    const uint64_t cutoff = limit/unsigned(base),remainder = limit%unsigned(base);
    const char* first = text;
    while (Digit(*text) < unsigned(base)) {
        const unsigned digit = Digit(*text++);
        if (result.magnitude > cutoff || (result.magnitude == cutoff && digit > remainder)) result.overflow = true;
        else if (!result.overflow) result.magnitude = result.magnitude*unsigned(base)+digit;
    }
    if (text != first) {
        if (end) *end = const_cast<char*>(text);
        if (result.overflow) { errno = ERANGE; result.magnitude = limit; }
    }
    return result;
}

int64_t Signed(const char* input,char** end,int base,uint64_t maximum) noexcept {
    const auto parsed = Parse(input,end,base,maximum,maximum+1);
    if (!parsed.negative) return int64_t(parsed.magnitude);
    // Avoid negating the minimum signed value or narrowing its magnitude first.
    return parsed.magnitude == (uint64_t(1)<<63) ? INT64_MIN : -int64_t(parsed.magnitude);
}

uint64_t Unsigned(const char* input,char** end,int base,uint64_t maximum) noexcept {
    const auto parsed = Parse(input,end,base,maximum,maximum);
    if (parsed.overflow) return maximum;
    return parsed.negative ? uint64_t(0)-parsed.magnitude : parsed.magnitude;
}
}

// Newlib libm uses the same hook as standard <errno.h>. It is private to each
// resident NRO and shared by that NRO's cooperative entries, without OS TLS.
extern "C" int* __errno() { return &error_value; }
extern "C" long strtol(const char* input,char** end,int base) {
    return static_cast<long>(Signed(input,end,base,LONG_MAX));
}
extern "C" unsigned long strtoul(const char* input,char** end,int base) {
    return static_cast<unsigned long>(Unsigned(input,end,base,ULONG_MAX));
}
extern "C" long long strtoll(const char* input,char** end,int base) {
    return static_cast<long long>(Signed(input,end,base,INT64_MAX));
}
extern "C" unsigned long long strtoull(const char* input,char** end,int base) {
    return static_cast<unsigned long long>(Unsigned(input,end,base,UINT64_MAX));
}
extern "C" int atoi(const char* input) { return static_cast<int>(strtol(input,nullptr,10)); }
extern "C" long atol(const char* input) { return strtol(input,nullptr,10); }
extern "C" long long atoll(const char* input) { return strtoll(input,nullptr,10); }
