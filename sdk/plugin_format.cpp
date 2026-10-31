// Buffer formatting for the explicit C profile. No streams, locale or allocation.
#include "shv/crt.h"
#include <stdint.h>
#include <limits.h>
#include <type_traits>
#ifdef SHVNX_PLUGIN_NUMERIC_RUNTIME
#include "third_party/ryu/ryu/ryu.h"
#endif

namespace {
class Output final {
public:
    Output(char* data,size_t capacity) noexcept : data_(data),capacity_(capacity) {}
    bool Repeat(char value,size_t count) noexcept {
        if (count > size_t(INT_MAX)-used_) return false;
        const size_t room = Room(), take = count < room ? count : room;
        for (size_t i = 0; i < take; ++i) data_[used_+i] = value;
        used_ += count;
        return true;
    }
    bool Append(const char* value,size_t count) noexcept {
        if (count > size_t(INT_MAX)-used_) return false;
        const size_t room = Room(), take = count < room ? count : room;
        for (size_t i = 0; i < take; ++i) data_[used_+i] = value[i];
        used_ += count;
        return true;
    }
    int Finish(bool success) noexcept {
        if (data_ && capacity_) data_[success ? (used_ < capacity_ ? used_ : capacity_-1) : 0] = 0;
        return success ? int(used_) : -1;
    }
private:
    char* data_;
    size_t capacity_,used_ = 0;
    size_t Room() const noexcept { return capacity_ && used_ < capacity_-1 ? capacity_-1-used_ : 0; }
};

struct Spec {
    bool left = false,plus = false,space = false,alternate = false,zero = false;
    int width = 0,precision = -1;
    enum Length { None, Char, Short, Long, LongLong, Maximum, Size, Difference } length = None;
};

bool Decimal(const char*& text,int& value) noexcept {
    value = 0;
    while (*text >= '0' && *text <= '9') {
        const int digit = *text++-'0';
        if (value > (INT_MAX-digit)/10) return false;
        value = value*10+digit;
    }
    return true;
}

bool Parse(const char*& text,Spec& spec,va_list& arguments) noexcept {
    for (;;) {
        if (*text == '-') spec.left = true;
        else if (*text == '+') spec.plus = true;
        else if (*text == ' ') spec.space = true;
        else if (*text == '#') spec.alternate = true;
        else if (*text == '0') spec.zero = true;
        else break;
        ++text;
    }
    if (*text == '*') {
        ++text; spec.width = va_arg(arguments,int);
        if (spec.width == INT_MIN) return false;
        if (spec.width < 0) { spec.left = true; spec.width = -spec.width; }
    } else if (!Decimal(text,spec.width)) return false;
    if (*text == '.') {
        ++text;
        if (*text == '*') { ++text; spec.precision = va_arg(arguments,int); }
        else if (!Decimal(text,spec.precision)) return false;
        if (spec.precision < 0) spec.precision = -1;
    }
    if (*text == 'h') { ++text; spec.length = Spec::Short; if (*text == 'h') { ++text; spec.length = Spec::Char; } }
    else if (*text == 'l') { ++text; spec.length = Spec::Long; if (*text == 'l') { ++text; spec.length = Spec::LongLong; } }
    else if (*text == 'j') { ++text; spec.length = Spec::Maximum; }
    else if (*text == 'z') { ++text; spec.length = Spec::Size; }
    else if (*text == 't') { ++text; spec.length = Spec::Difference; }
    return true;
}

int64_t Signed(va_list& arguments,Spec::Length length) noexcept {
    switch (length) {
        case Spec::Char: return static_cast<signed char>(va_arg(arguments,int));
        case Spec::Short: return static_cast<short>(va_arg(arguments,int));
        case Spec::Long: return va_arg(arguments,long);
        case Spec::LongLong: return va_arg(arguments,long long);
        case Spec::Maximum: return va_arg(arguments,intmax_t);
        case Spec::Size: return va_arg(arguments,std::make_signed_t<size_t>);
        case Spec::Difference: return va_arg(arguments,ptrdiff_t);
        default: return va_arg(arguments,int);
    }
}

uint64_t Unsigned(va_list& arguments,Spec::Length length) noexcept {
    switch (length) {
        case Spec::Char: return static_cast<unsigned char>(va_arg(arguments,int));
        case Spec::Short: return static_cast<unsigned short>(va_arg(arguments,int));
        case Spec::Long: return va_arg(arguments,unsigned long);
        case Spec::LongLong: return va_arg(arguments,unsigned long long);
        case Spec::Maximum: return va_arg(arguments,uintmax_t);
        case Spec::Size: return va_arg(arguments,size_t);
        case Spec::Difference: return va_arg(arguments,std::make_unsigned_t<ptrdiff_t>);
        default: return va_arg(arguments,unsigned int);
    }
}

bool Field(Output& output,const Spec& spec,const char* prefix,size_t prefix_bytes,
        const char* body,size_t body_bytes,size_t leading_zeros,bool numeric) noexcept {
    const size_t bytes = prefix_bytes+leading_zeros+body_bytes;
    const size_t padding = size_t(spec.width) > bytes ? size_t(spec.width)-bytes : 0;
    const bool zero_padding = numeric && spec.zero && !spec.left;
    return (spec.left || zero_padding || output.Repeat(' ',padding)) &&
        output.Append(prefix,prefix_bytes) &&
        (!zero_padding || output.Repeat('0',padding)) &&
        output.Repeat('0',leading_zeros) && output.Append(body,body_bytes) &&
        (!spec.left || output.Repeat(' ',padding));
}

bool Integer(Output& output,Spec spec,uint64_t value,bool negative,unsigned radix,
        bool uppercase,bool signed_value,bool pointer) noexcept {
    char storage[65],prefix[3]; size_t used = 0,prefix_bytes = 0;
    const bool nonzero = value != 0;
    const char* alphabet = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    if (value || spec.precision != 0 || pointer) {
        do { storage[used++] = alphabet[value%radix]; value /= radix; } while (value);
    }
    for (size_t i = 0; i < used/2; ++i) { const char saved = storage[i]; storage[i] = storage[used-1-i]; storage[used-1-i] = saved; }
    if (signed_value) {
        if (negative) prefix[prefix_bytes++] = '-';
        else if (spec.plus) prefix[prefix_bytes++] = '+';
        else if (spec.space) prefix[prefix_bytes++] = ' ';
    }
    if (pointer || (spec.alternate && radix == 16 && nonzero)) {
        prefix[prefix_bytes++] = '0'; prefix[prefix_bytes++] = uppercase ? 'X' : 'x';
    }
    size_t zeros = spec.precision >= 0 && size_t(spec.precision) > used ? size_t(spec.precision)-used : 0;
    if (spec.alternate && radix == 8 && !zeros && (!used || storage[0] != '0')) zeros = 1;
    if (spec.precision >= 0) spec.zero = false;
    return Field(output,spec,prefix,prefix_bytes,storage,used,zeros,true);
}

#ifdef SHVNX_PLUGIN_NUMERIC_RUNTIME
// Precision is capped before entering Ryu, bounding scratch and conversion work.
constexpr int MaxFloatPrecision = 64;
size_t General(double value,int precision,bool alternate,char* output) noexcept {
    char scientific[96],digits[65];
    const int count = d2exp_buffered_n(value,unsigned(precision-1),scientific);
    size_t at = 0,used = 0;
    while (at < size_t(count) && scientific[at] != 'e') {
        if (scientific[at] != '.') digits[used++] = scientific[at];
        ++at;
    }
    ++at; const bool negative = scientific[at++] == '-'; int exponent = 0;
    while (at < size_t(count)) exponent = exponent*10+(scientific[at++]-'0');
    if (negative) exponent = -exponent;
    if (!alternate) while (used > 1 && digits[used-1] == '0') --used;
    size_t out = 0;
    if (exponent < -4 || exponent >= precision) {
        output[out++] = digits[0];
        if (used > 1 || alternate) output[out++] = '.';
        for (size_t i = 1; i < used; ++i) output[out++] = digits[i];
        output[out++] = 'e'; output[out++] = exponent < 0 ? '-' : '+';
        const unsigned magnitude = unsigned(exponent < 0 ? -exponent : exponent);
        if (magnitude >= 100) output[out++] = char('0'+magnitude/100);
        output[out++] = char('0'+magnitude/10%10); output[out++] = char('0'+magnitude%10);
    } else {
        const int point = exponent+1;
        if (point <= 0) {
            output[out++] = '0'; output[out++] = '.';
            for (int i = 0; i < -point; ++i) output[out++] = '0';
            for (size_t i = 0; i < used; ++i) output[out++] = digits[i];
        } else {
            for (size_t i = 0; i < used || int(i) < point; ++i) {
                if (int(i) == point) output[out++] = '.';
                output[out++] = i < used ? digits[i] : '0';
            }
            if (alternate && used <= size_t(point)) output[out++] = '.';
        }
    }
    return out;
}

bool Floating(Output& output,Spec spec,double value,char conversion) noexcept {
    if (spec.length != Spec::None && spec.length != Spec::Long) return false;
    const char lower = conversion >= 'A' && conversion <= 'Z' ? char(conversion+'a'-'A') : conversion;
    int precision = spec.precision < 0 ? 6 : spec.precision;
    if (lower == 'g' && !precision) precision = 1;
    if (precision > MaxFloatPrecision) return false;
    uint64_t bits; __builtin_memcpy(&bits,&value,sizeof(bits));
    char prefix[1]; size_t prefix_bytes = 0;
    if (bits >> 63) prefix[prefix_bytes++] = '-';
    else if (spec.plus) prefix[prefix_bytes++] = '+';
    else if (spec.space) prefix[prefix_bytes++] = ' ';
    bits &= ~(uint64_t(1)<<63); __builtin_memcpy(&value,&bits,sizeof(bits));
    char text[384]; size_t bytes;
    const bool finite = (bits&0x7ff0000000000000ull) != 0x7ff0000000000000ull;
    if (!finite) {
        const char* word = bits&0xfffffffffffffull ? "nan" : "inf";
        for (size_t i = 0; i < 3; ++i) text[i] = word[i];
        bytes = 3; spec.zero = false;
    } else if (lower == 'g') bytes = General(value,precision,spec.alternate,text);
    else {
        bytes = size_t(lower == 'f' ? d2fixed_buffered_n(value,unsigned(precision),text) : d2exp_buffered_n(value,unsigned(precision),text));
        if (spec.alternate && !precision) {
            const size_t insert = lower == 'f' ? bytes : 1;
            for (size_t i = bytes; i > insert; --i) text[i] = text[i-1];
            text[insert] = '.'; ++bytes;
        }
    }
    if (conversion >= 'A' && conversion <= 'Z') for (size_t i = 0; i < bytes; ++i)
        if (text[i] >= 'a' && text[i] <= 'z') text[i] = char(text[i]+'A'-'a');
    return Field(output,spec,prefix,prefix_bytes,text,bytes,0,finite);
}
#endif

bool Format(Output& output,const char* format,va_list& arguments) noexcept {
    while (*format) {
        const char* start = format;
        while (*format && *format != '%') ++format;
        if (!output.Append(start,size_t(format-start))) return false;
        if (!*format) break;
        ++format;
        if (*format == '%') { ++format; if (!output.Repeat('%',1)) return false; continue; }
        Spec spec;
        if (!Parse(format,spec,arguments) || !*format) return false;
        const char conversion = *format++;
        if (conversion == 'd' || conversion == 'i') {
            const int64_t value = Signed(arguments,spec.length);
            const uint64_t magnitude = value < 0 ? uint64_t(0)-uint64_t(value) : uint64_t(value);
            if (!Integer(output,spec,magnitude,value < 0,10,false,true,false)) return false;
        } else if (conversion == 'u' || conversion == 'o' || conversion == 'x' || conversion == 'X') {
            const unsigned radix = conversion == 'o' ? 8 : conversion == 'u' ? 10 : 16;
            if (!Integer(output,spec,Unsigned(arguments,spec.length),false,radix,conversion == 'X',false,false)) return false;
        } else if (conversion == 'p') {
            if (spec.length != Spec::None || !Integer(output,spec,reinterpret_cast<uintptr_t>(va_arg(arguments,void*)),false,16,false,false,true)) return false;
        } else if (conversion == 's' || conversion == 'c') {
            if (spec.length != Spec::None) return false;
            char character; const char* text; size_t bytes = 0;
            if (conversion == 'c') { character = char(va_arg(arguments,int)); text = &character; bytes = 1; }
            else {
                text = va_arg(arguments,const char*); if (!text) text = "(null)";
                const size_t limit = spec.precision < 0 ? size_t(INT_MAX) : size_t(spec.precision);
                while (bytes < limit && text[bytes]) ++bytes;
                if (spec.precision < 0 && bytes == size_t(INT_MAX) && text[bytes]) return false;
            }
            if (!Field(output,spec,"",0,text,bytes,0,false)) return false;
        } else {
#ifdef SHVNX_PLUGIN_NUMERIC_RUNTIME
            if ((conversion != 'f' && conversion != 'F' && conversion != 'e' && conversion != 'E' && conversion != 'g' && conversion != 'G') ||
                    (spec.length != Spec::None && spec.length != Spec::Long) ||
                    !Floating(output,spec,va_arg(arguments,double),conversion)) return false;
#else
            return false;
#endif
        }
    }
    return true;
}
}

extern "C" int vsnprintf(char* output,size_t capacity,const char* format,va_list arguments) {
    if ((!output && capacity) || !format) { if (output && capacity) *output = 0; return -1; }
    Output destination(output,capacity);
    va_list copy; va_copy(copy,arguments);
    const bool success = Format(destination,format,copy);
    va_end(copy);
    return destination.Finish(success);
}

extern "C" int snprintf(char* output,size_t capacity,const char* format,...) {
    va_list arguments; va_start(arguments,format);
    const int result = vsnprintf(output,capacity,format,arguments);
    va_end(arguments);
    return result;
}
