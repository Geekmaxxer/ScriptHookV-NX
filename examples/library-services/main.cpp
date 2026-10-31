#include "script.h"
#include "shv/crt.h"
#include <cmath>
#include <errno.h>
#include <stdint.h>
#include <string.h>

namespace {
uint32_t run;

bool IntegerChecks() {
    char* end = nullptr;
    errno = 0;
    const char hexadecimal[] = " 0x2699tail";
    if (strtol(hexadecimal,&end,0) != 0x2699 || strcmp(end,"tail") || errno) return false;
    if (strtoull("zz!",&end,36) != 1295 || strcmp(end,"!")) return false;
    const char invalid[] = "invalid";
    errno = 123;
    if (strtol(invalid,&end,10) != 0 || end != invalid || errno != 123) return false;
    return atoi(" -61tail") == -61 && atol("2699tail") == 2699 && atoll("-2699tail") == -2699;
}

bool DecimalChecks() {
    char* end = nullptr;
    errno = 0;
    if (strtod(" +12.5tail",&end) != 12.5 || strcmp(end,"tail") || errno) return false;
    const float rounded = strtof("1.0000000596046447753906250000000001",&end);
    uint32_t bits = 0; memcpy(&bits,&rounded,sizeof(bits));
    return bits == 0x3f800001u && !*end && atof("-1.25tail") == -1.25;
}

bool ErrorChecks() {
    char* end = nullptr;
    errno = 0;
    if (strtoll("-9223372036854775809tail",&end,10) != INT64_MIN ||
            strcmp(end,"tail") || errno != ERANGE) return false;
    errno = 0;
    const double range = strtod("1e99999tail",&end);
    if (!std::isinf(range) || range < 0 || strcmp(end,"tail") || errno != ERANGE) return false;
    errno = 0;
    const char input[] = "12";
    return strtol(input,&end,1) == 0 && end == input && errno == EINVAL;
}

bool FormatTwice(char* output,size_t capacity,const char* format,...) {
    va_list arguments; va_start(arguments,format);
    const int counted = vsnprintf(nullptr,0,format,arguments);
    const int written = vsnprintf(output,capacity,format,arguments);
    va_end(arguments);
    return counted == written && written >= 0 && size_t(written) < capacity;
}

void Main() {
    ++run;
    const bool integers = IntegerChecks(),decimals = DecimalChecks(),errors = ErrorChecks();
    bool geometry = true;
    for (unsigned i = 0; i < 32; ++i) {
        const double angle = double(i)*0.125;
        const double x = std::cos(angle),y = std::sin(angle);
        if (std::fabs(std::hypot(x,y)-1.0) >= 1e-12 || std::fabs(x*x+y*y-1.0) >= 1e-12) geometry = false;
        WAIT(0);
    }
    const double distance = std::hypot(std::hypot(3.0,4.0),12.0);
    geometry = geometry && std::fabs(distance-13.0) < 1e-12 &&
        std::fabs(std::atan2(1.0,1.0)-0.7853981633974483) < 1e-12;
    errno = 0;
    const double domain = std::sqrt(-1.0);
    bool math_errors = std::isnan(domain) && errno == EDOM;
    errno = 0;
    const double overflow = std::exp(10000.0);
    math_errors = math_errors && std::isinf(overflow) && overflow > 0 && errno == ERANGE;
    char formatted[96],small[5];
    const bool formatting = FormatTwice(formatted,sizeof(formatted),
        "run=%u parsed=%ld distance=%.2f",run,strtol("2699 items",nullptr,10),distance);
    const bool truncated = snprintf(small,sizeof(small),"%08x",2699u) == 8 && memcmp(small,"0000",5) == 0;
    const char rejected[] = {'%','n',0}; int unchanged = 2699;
    const bool refusal = snprintf(small,sizeof(small),rejected,&unchanged) == -1 && small[0] == 0 && unchanged == 2699;
    ShvNxMemoryStats memory{};
    const bool clean = scriptGetMemoryStats(&memory) && !memory.allocations;
    char report[640];
    const int bytes = snprintf(report,sizeof(report),
        "run=%u\nfailed=%u\ninteger_verified=%u\ndecimal_verified=%u\nerrno_verified=%u\ngeometry_verified=%u\nmath_errors_verified=%u\nformat_verified=%u\ntruncation_verified=%u\nrejection_verified=%u\nowned_allocations=%zu\nformatted=%s\n",
        run,unsigned(!(integers && decimals && errors && geometry && math_errors && formatting && truncated && refusal && clean)),
        unsigned(integers),unsigned(decimals),unsigned(errors),unsigned(geometry),unsigned(math_errors),unsigned(formatting),
        unsigned(truncated),unsigned(refusal),memory.allocations,formatting ? formatted : "unavailable");
    if (bytes >= 0 && size_t(bytes) < sizeof(report)) (void)scriptSaveFile("result.txt",report,size_t(bytes));
}
}
SHV_NX_PLUGIN("library-services-example",Main)
