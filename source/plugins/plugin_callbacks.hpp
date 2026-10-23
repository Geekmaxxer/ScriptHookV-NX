#pragma once
#include "extensions.h"
namespace ScriptHook {
class PluginCallbacks {
public:
    enum Kind : uint32_t { Control,Render,Present,Keyboard };
    static bool Register(void* owner,uint32_t kind,uintptr_t function,uint32_t group,uint32_t control,bool add,uint32_t frame);
    static void Enable(void* owner,bool enabled);
    static void Forget(void* owner);
    static bool Active(void* owner=nullptr);
    static uint32_t Failures(void* owner);
    static void TickInput(uint32_t frame);
    static void TickRender(uint32_t frame);
};
}
