#include "script.h"
#include "shv/crt.h"
#include "shv/text.h"
#include <string.h>

namespace {
uint32_t run;

bool RepeatedArguments(char* output,size_t capacity,const char* format,...) {
    va_list arguments; va_start(arguments,format);
    const int required = vsnprintf(nullptr,0,format,arguments);
    const int written = vsnprintf(output,capacity,format,arguments);
    va_end(arguments);
    return required == written && written >= 0 && size_t(written) < capacity;
}

void Main() {
    ++run;
    char formatted[96],small[5];
    const bool arguments = RepeatedArguments(formatted,sizeof(formatted),
        "profile=%s count=%zu value=%+.2f%%","demo",size_t(2699),12.5) &&
        strcmp(formatted,"profile=demo count=2699 value=+12.50%") == 0;
    const bool truncated = snprintf(small,sizeof(small),"%08x",2699u) == 8 &&
        memcmp(small,"0000",5) == 0;
    int untouched = 2699;
    const char unsupported[] = {'%','n',0};
    const bool refused = snprintf(small,sizeof(small),unsupported,&untouched) == -1 &&
        small[0] == 0 && untouched == 2699;
    ShvNxMemoryStats memory{};
    const bool empty = scriptGetMemoryStats(&memory) && !memory.allocations;
    ScriptHook::Util::TextBuffer<320> report;
    report.Add("run=").UInt(run).Add("\nfailed=").UInt(!(arguments && truncated && refused && empty))
        .Add("\nva_list_verified=").UInt(arguments).Add("\ntruncation_verified=").UInt(truncated)
        .Add("\nrejected_format_verified=").UInt(refused).Add("\nowned_allocations=").UInt(uint32_t(memory.allocations))
        .Add("\nformatted=").Add(arguments ? formatted : "unavailable").Add("\n");
    (void)scriptSaveFile("result.txt",report.CStr(),report.Size());
}
}
SHV_NX_PLUGIN("formatted-text-example",Main)
