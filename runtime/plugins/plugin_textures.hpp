#pragma once
#include "extensions.h"
namespace ScriptHook {
class PluginTextures {
public:
    static int Create(void* owner,const char* id,const char* name);
    static bool Draw(void* owner,const ShvNxTextureDraw* draw);
    static void Stop(void* owner);
    static bool Active(void* owner=nullptr);
    static size_t Bytes(void* owner);
    static void Frame();
};
}
