#pragma once
#include "memory.hpp"
namespace ScriptHook {
struct KeyboardState { uint64_t sampling,modifiers,keys[4]; };
static_assert(sizeof(KeyboardState)==48);
class EngineApi {
public:
    static bool Verified();
    static bool Context();
    static int Enumerate(uint32_t kind,int32_t* out,uint32_t capacity);
    static uintptr_t EntityAddress(int32_t handle);
    static uint64_t* Global(uint32_t index);
    static bool KeyboardSupported();
    static bool Keyboard(KeyboardState& out);
    static uintptr_t CreateTexture(const void* bgra,uint32_t width,uint32_t height);
    static bool Sprite(uintptr_t texture,float x,float y,float width,float height,float degrees,int r,int g,int b,int a);
};
}
