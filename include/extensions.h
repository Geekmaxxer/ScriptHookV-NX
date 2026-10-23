#pragma once
#include "main.h"

#define SHV_NX_EXTENSIONS_ABI_VERSION 1u
#define SHV_NX_CAP_MULTIPLE_SCRIPTS (UINT64_C(1) << 0)
#define SHV_NX_CAP_ENTITY_ENUMERATION (UINT64_C(1) << 1)
#define SHV_NX_CAP_SCRIPT_GLOBALS (UINT64_C(1) << 2)
#define SHV_NX_CAP_ENTITY_POINTERS (UINT64_C(1) << 3)
#define SHV_NX_CAP_CONTROL_CALLBACKS (UINT64_C(1) << 4)
#define SHV_NX_CAP_RENDER_CALLBACKS (UINT64_C(1) << 5)
#define SHV_NX_CAP_KEYBOARD_CALLBACKS (UINT64_C(1) << 6)
#define SHV_NX_CAP_CUSTOM_TEXTURES (UINT64_C(1) << 7)
#ifdef __cplusplus
extern "C" {
#endif

typedef enum ShvNxInputKind { ShvNxInput_Pressed=1,ShvNxInput_Released=2,ShvNxInput_Repeat=3,ShvNxInput_Analog=4 } ShvNxInputKind;
typedef struct ShvNxInputEvent { uint32_t size,frame,group,control,kind;float value; } ShvNxInputEvent;
typedef struct ShvNxRenderFrame { uint32_t size,frame,width,height;uint64_t ticks; } ShvNxRenderFrame;
typedef void (*ControlHandler)(const ShvNxInputEvent*);
typedef void (*RenderCallback)(const ShvNxRenderFrame*);
typedef void (*PresentCallback)(void*);
typedef void (*KeyboardHandler)(DWORD,WORD,BYTE,BOOL,BOOL,BOOL,BOOL);
typedef struct ShvNxTextureDraw {
    int32_t id,index,level,time;
    float sizeX,sizeY,centerX,centerY,posX,posY,rotation,heightScale,r,g,b,a;
} ShvNxTextureDraw;

// Optional, independently versioned extension table. The original plugin/API
// descriptors remain byte-for-byte ABI 1; old NROs do not import this table.
typedef struct ShvNxExtensions {
    uint32_t size, abi;
    uint64_t capabilities;
    bool (*register_script)(void*, ScriptFunction);
    bool (*unregister_script)(void*, ScriptFunction);
    int (*enumerate)(void*,uint32_t,int32_t*,uint32_t);
    uint64_t* (*global)(void*,uint32_t);
    uintptr_t (*entity)(void*,int32_t);
    bool (*callback)(void*,uint32_t,uintptr_t,uint32_t,uint32_t,bool);
    int (*create_texture)(void*,const char*);
    bool (*draw_texture)(void*,const ShvNxTextureDraw*);
} ShvNxExtensions;

SHV_NX_API bool shvPluginBindExtensions(const ShvNxExtensions* extensions);
SHV_NX_API bool scriptRegisterAdditionalThread(ScriptFunction function);
SHV_NX_API void scriptUnregisterAll(void);
SHV_NX_API bool scriptUnregisterChecked(ScriptFunction function);
SHV_NX_API uint64_t getScriptHookCapabilities(void);
SHV_NX_API int worldGetAllPeds(int* array,int capacity);
SHV_NX_API int worldGetAllVehicles(int* array,int capacity);
SHV_NX_API int worldGetAllObjects(int* array,int capacity);
SHV_NX_API int worldGetAllPickups(int* array,int capacity);
SHV_NX_API uint64_t* getGlobalPtr(int index);
SHV_NX_API BYTE* getScriptHandleBaseAddress(int handle);
SHV_NX_API bool controlHandlerRegister(ControlHandler handler,uint32_t group,uint32_t control);
SHV_NX_API bool controlHandlerUnregister(ControlHandler handler,uint32_t group,uint32_t control);
SHV_NX_API bool renderCallbackRegister(RenderCallback handler);
SHV_NX_API bool renderCallbackUnregister(RenderCallback handler);
// NX frame callback: argument is ShvNxRenderFrame*, never an IDXGISwapChain*.
SHV_NX_API void presentCallbackRegister(PresentCallback handler);
SHV_NX_API bool presentCallbackRegisterChecked(PresentCallback handler);
SHV_NX_API void presentCallbackUnregister(PresentCallback handler);
// USB keyboard, US virtual-key translation; scanCode is a USB HID usage.
SHV_NX_API void keyboardHandlerRegister(KeyboardHandler handler);
SHV_NX_API bool keyboardHandlerRegisterChecked(KeyboardHandler handler);
SHV_NX_API void keyboardHandlerUnregister(KeyboardHandler handler);
// PNG assets are read from assets/<plugin-id>/<filename>. IDs are per-owner.
SHV_NX_API int createTexture(const char* filename);
SHV_NX_API bool drawTextureChecked(const ShvNxTextureDraw* draw);
SHV_NX_API void drawTexture(int id,int index,int level,int time,
    float sizeX,float sizeY,float centerX,float centerY,float posX,float posY,
    float rotation,float screenHeightScaleFactor,float r,float g,float b,float a);

#ifdef __cplusplus
}
// PC HMODULE values have no meaning on Switch. Registration always belongs to
// the current NRO, never to the value supplied in this compatibility argument.
inline void scriptRegister(void*, ScriptFunction function) { scriptRegister(function); }
inline void scriptUnregister(void*) { scriptUnregisterAll(); }
inline bool scriptRegisterAdditionalThread(void*, ScriptFunction function) {
    return scriptRegisterAdditionalThread(function);
}
#endif
