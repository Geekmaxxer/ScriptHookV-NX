#pragma once
#include <stdint.h>
#include <stddef.h>
namespace ScriptHook {
class NativeManifest {
public:
    static bool Import(const void* data,size_t size);
    static bool Repair(void* thread);
    static uint32_t Count();
};
}
