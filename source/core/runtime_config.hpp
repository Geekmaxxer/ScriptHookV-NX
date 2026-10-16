#pragma once
#include "runtime_defaults.h"
#include <stdint.h>
#include <stddef.h>
namespace ScriptHook {
class RuntimeConfig {
public:
    static void LoadOnce(); // First valid game-script tick, never module startup.
    static size_t Residents();
    static size_t Scripts();
    static size_t MemoryBytes();
    static uint32_t State(); // 0 not read, 1 absent, 2 read, 3 read error.
    static uint32_t Error();
    static uint32_t AppliedMask(); // bits 0/1/2: residents/scripts/memory.
    static uint32_t InvalidMask();
    static uint32_t StackError();
};
}
