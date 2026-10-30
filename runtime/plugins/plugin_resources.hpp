#pragma once
#include "plugin_resources.h"

namespace ScriptHook {
class PluginResources {
public:
    static ShvNxResourceHandle Register(void* owner, ShvNxResourceCleanup cleanup, void* context);
    static bool Release(void* owner, ShvNxResourceHandle handle);
    static bool Forget(void* owner, ShvNxResourceHandle handle);
    static bool Active(void* owner, ShvNxResourceHandle handle);
    static bool Pinned(void* owner, const void* pointer);
    static uint32_t Count(void* owner);
    static uint64_t Failures(void* owner);
    static bool Drain(void* owner);
};
}
