#pragma once
#include <stdint.h>
#include <stddef.h>

namespace ScriptHook {

class ScriptThreadHook {
public:
    static bool s_hooked;
    static uintptr_t s_originalTick;

    static bool Install();
};

} // namespace ScriptHook
