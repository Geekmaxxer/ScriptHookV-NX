#pragma once
#include <stddef.h>
#include <stdint.h>

// Opt in with c_runtime=true and numeric_runtime=true. No hosted locale/errno.
namespace ScriptHook::Numeric {
enum class Status:uint32_t { Ok, Invalid, OutOfRange, NonFinite, BufferTooSmall, InputTooLong };
constexpr size_t MaxInputBytes=4096;
// Whole ASCII decimal token; surrounding ASCII whitespace and one sign allowed.
// Output is unchanged on error. Hex, embedded NUL, non-finite and suffixes fail.
Status ParseFloat(const char* input,size_t bytes,float& output) noexcept;
Status ParseDouble(const char* input,size_t bytes,double& output) noexcept;
// Shortest roundtripping scientific decimal, finite values only, including -0.
// Room for NUL is required. Failure leaves output storage unchanged; bytes=0.
Status FormatFloat(float value,char* output,size_t capacity,size_t& bytes) noexcept;
Status FormatDouble(double value,char* output,size_t capacity,size_t& bytes) noexcept;
namespace Detail {
// Pinned pugixml-only decimal prefix and %.*g bridges. Not C stdlib replacements.
double XmlNumber(const char* input,char** end) noexcept;
int XmlPrint(char* output,size_t capacity,const char* format,int precision,double value) noexcept;
}
}
