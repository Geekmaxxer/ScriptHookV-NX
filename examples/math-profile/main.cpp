#include "script.h"
#include "shv/crt.h"
#include <cmath>
#include <errno.h>
#include <string.h>

namespace {
uint32_t run;

bool Near(double value,double expected) {
    return std::fabs(value-expected) < 1e-12;
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
    bool geometry = true;
    for (unsigned i = 0; i < 32; ++i) {
        const double angle = double(i)*0.125;
        const double x = std::cos(angle),y = std::sin(angle);
        if (!Near(std::hypot(x,y),1.0) || !Near(x*x+y*y,1.0)) geometry = false;
        WAIT(0);
    }
    const double distance = std::hypot(std::hypot(3.0,4.0),12.0);
    geometry = geometry && Near(distance,13.0) && Near(std::atan2(1.0,1.0),0.7853981633974483);
    errno = 0;
    const double invalid = std::sqrt(-1.0);
    const bool domain = std::isnan(invalid) && errno == EDOM;
    errno = 0;
    const double overflow = std::exp(10000.0);
    const bool range = std::isinf(overflow) && overflow > 0 && errno == ERANGE;
    char formatted[96],small[5];
    const bool formatting = FormatTwice(formatted,sizeof(formatted),
        "run=%u distance=%.2f heading=%.6f",run,distance,std::atan2(1.0,1.0));
    const bool truncated = snprintf(small,sizeof(small),"%08x",2699u) == 8 && memcmp(small,"0000",5) == 0;
    const char rejected[] = {'%','n',0}; int unchanged = 2699;
    const bool refusal = snprintf(small,sizeof(small),rejected,&unchanged) == -1 && small[0] == 0 && unchanged == 2699;
    ShvNxMemoryStats memory{};
    const bool clean = scriptGetMemoryStats(&memory) && !memory.allocations;
    char report[512];
    const int bytes = snprintf(report,sizeof(report),
        "run=%u\nfailed=%u\ngeometry_verified=%u\ndomain_verified=%u\nrange_verified=%u\nformat_verified=%u\ntruncation_verified=%u\nrejection_verified=%u\nowned_allocations=%zu\nformatted=%s\n",
        run,unsigned(!(geometry && domain && range && formatting && truncated && refusal && clean)),
        unsigned(geometry),unsigned(domain),unsigned(range),unsigned(formatting),unsigned(truncated),unsigned(refusal),memory.allocations,formatting ? formatted : "unavailable");
    if (bytes >= 0 && size_t(bytes) < sizeof(report)) (void)scriptSaveFile("result.txt",report,size_t(bytes));
}
}
SHV_NX_PLUGIN("math-profile-example",Main)
