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
    static size_t ModuleImageBytes();
    static size_t ModuleBssBytes();
    static uint32_t State(); // 0 not read, 1 absent, 2 read, 3 read error.
    static uint32_t Error();
    static uint32_t AppliedMask(); // bits 0..4: residents/scripts/memory/image/BSS.
    static uint32_t InvalidMask();
    static uint32_t StackError();
    static void SetStackError(uint32_t error);
    static void* AllocateMetadata(size_t bytes);
    static size_t MetadataBytes();
    static uint32_t MetadataError();
    static uint32_t AutomaticMask();
};
}
