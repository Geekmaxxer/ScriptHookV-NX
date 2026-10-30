#include "plugin_callbacks.hpp"
#include "plugin_loader.hpp"
#include "../script/script_manager.hpp"
#include "../core/engine_api.hpp"
#include "natives.h"
namespace ScriptHook {
namespace {
constexpr size_t maximum=128,per_owner=16;
struct Entry {void* owner;uintptr_t function;uint64_t repeat_at;uint32_t kind,group,control,born,serial,failures;float value;bool down,enabled;};
Entry entries[maximum];uint32_t next_serial=1;
struct Call {Entry snapshot;ShvNxInputEvent input;ShvNxRenderFrame render;uint32_t key;uint8_t scan;bool up,was_down,alt,extended;};
Call* current;
void invoke(){
    const auto& c=*current;
    if(c.snapshot.kind==PluginCallbacks::Control)reinterpret_cast<ControlHandler>(c.snapshot.function)(&c.input);
    else if(c.snapshot.kind==PluginCallbacks::Render)reinterpret_cast<RenderCallback>(c.snapshot.function)(&c.render);
    else if(c.snapshot.kind==PluginCallbacks::Present)reinterpret_cast<PresentCallback>(c.snapshot.function)(&current->render);
    else reinterpret_cast<KeyboardHandler>(c.snapshot.function)(c.key,1,c.scan,c.extended,c.alt,c.was_down,c.up);
}
bool dispatch(Entry& entry,Call& call){
    if(!entry.enabled||!entry.function||entry.serial!=call.snapshot.serial)return false;
    current=&call;const auto started=sh_get_tick();const bool ok=ScriptManager::RunCleanup(invoke);current=nullptr;
    PluginLoader::RecordTime(call.snapshot.owner,1,sh_get_tick()-started);
    if(!ok&&entry.serial==call.snapshot.serial){++entry.failures;entry.enabled=false;PluginLoader::CallbackFailed();}
    return ok;
}
uint32_t virtual_key(uint32_t hid){
    if(hid>=4&&hid<=29)return 'A'+hid-4;
    if(hid>=30&&hid<=38)return '1'+hid-30;
    if(hid==39)return '0';
    if(hid>=58&&hid<=69)return 0x70+hid-58;
    if(hid>=104&&hid<=115)return 0x7c+hid-104;
    if(hid>=89&&hid<=97)return 0x61+hid-89;
    constexpr uint8_t specials[]={0x0d,0x1b,0x08,0x09,0x20,0xbd,0xbb,0xdb,0xdd,0xdc,0xdc,0xba,0xde,0xc0,0xbc,0xbe,0xbf,0x14};
    if(hid>=40&&hid<58)return specials[hid-40];
    constexpr uint8_t navigation[]={0x2c,0x91,0x13,0x2d,0x24,0x21,0x2e,0x23,0x22,0x27,0x25,0x28,0x26,0x90,0x6f,0x6a,0x6d,0x6b,0x0d};
    if(hid>=70&&hid<=88)return navigation[hid-70];
    if(hid==98)return 0x60;
    if(hid==99)return 0x6e;
    if(hid==101)return 0x5d;
    constexpr uint8_t modifiers[]={0xa2,0xa0,0xa4,0x5b,0xa3,0xa1,0xa5,0x5c};
    return hid>=224&&hid<=231?modifiers[hid-224]:0;
}
KeyboardState previous_keyboard{};uint64_t key_repeat[256];
}
bool PluginCallbacks::Register(void* owner,uint32_t kind,uintptr_t function,uint32_t group,uint32_t control,bool add,uint32_t frame){
    if(!owner||!function||kind>Keyboard||(kind==Control&&(group>2||control>359)))return false;
    if(kind==Keyboard&&!EngineApi::KeyboardSupported())return false;
    Entry* free=nullptr;size_t used=0;
    for(auto& e:entries){
        if(!e.function){if(!free)free=&e;continue;}
        if(e.owner!=owner)continue;
        ++used;
        if(e.kind==kind&&e.function==function&&(kind!=Control||(e.group==group&&e.control==control))){
            if(!add)e={};else e.enabled=true;
            return true;
        }
    }
    if(!add||!free||used>=per_owner)return false;
    *free={owner,function,0,kind,group,control,frame,next_serial++,0,0.f,false,true};
    if(!next_serial)++next_serial;
    return true;
}
void PluginCallbacks::Enable(void* owner,bool enabled){for(auto& e:entries)if(e.owner==owner){e.enabled=enabled;e.down=false;e.value=0.f;e.repeat_at=0;}}
void PluginCallbacks::Forget(void* owner){for(auto& e:entries)if(e.owner==owner)e={};}
bool PluginCallbacks::Active(void* owner){for(const auto& e:entries)if(e.enabled&&e.function&&(!owner||owner==e.owner))return true;return false;}
uint32_t PluginCallbacks::Failures(void* owner){uint32_t n=0;for(const auto& e:entries)if(e.owner==owner)n+=e.failures;return n;}
void PluginCallbacks::TickInput(uint32_t frame){
    const auto ceiling=next_serial;const auto now=sh_get_tick();bool keyboard=false;
    for(auto& e:entries){
        if(!e.enabled||!e.function||e.serial>=ceiling||e.born==frame)continue;
        if(e.kind==Keyboard){keyboard=true;continue;}
        if(e.kind!=Control)continue;
        const auto identity=e.serial;
        const bool down=Natives::PAD::IS_DISABLED_CONTROL_PRESSED(int32_t(e.group),int32_t(e.control));
        const bool pressed_ok=getNativeCallStatus()==NativeCall_Ok;
        const float value=Natives::PAD::GET_DISABLED_CONTROL_NORMAL(int32_t(e.group),int32_t(e.control));
        if(!pressed_ok||getNativeCallStatus()!=NativeCall_Ok||!__builtin_isfinite(value))continue;
        uint32_t kind=0;
        if(down!=e.down){kind=down?ShvNxInput_Pressed:ShvNxInput_Released;e.repeat_at=now+19200*500;}
        else if(down&&now>=e.repeat_at){kind=ShvNxInput_Repeat;e.repeat_at=now+19200*100;}
        else if(value!=e.value)kind=ShvNxInput_Analog;
        e.down=down;e.value=value;
        if(kind&&e.serial==identity){Call call{};call.snapshot=e;call.input={sizeof(ShvNxInputEvent),frame,e.group,e.control,kind,value};dispatch(e,call);}
    }
    if(!keyboard)return;
    KeyboardState state{};if(!EngineApi::Keyboard(state))return;
    for(uint32_t hid=0;hid<256;++hid){
        const auto key=virtual_key(hid);if(!key)continue;
        const bool down=(state.keys[hid>>6]&(UINT64_C(1)<<(hid&63)))!=0;
        const bool before=(previous_keyboard.keys[hid>>6]&(UINT64_C(1)<<(hid&63)))!=0;
        if(down==before&&(!down||now<key_repeat[hid]))continue;
        key_repeat[hid]=now+19200*(down&&!before?500:100);
        for(auto& e:entries)if(e.enabled&&e.function&&e.kind==Keyboard&&e.serial<ceiling&&e.born!=frame){
            Call call{};call.snapshot=e;call.key=key;call.scan=uint8_t(hid);call.up=!down;call.was_down=before;
            call.alt=(state.modifiers&12)!=0;call.extended=(hid>=73&&hid<=82)||hid==84||hid==88||hid==228||hid==230;
            dispatch(e,call);
        }
    }
    previous_keyboard=state;
}
void PluginCallbacks::TickRender(uint32_t frame){
    bool requested=false;
    for(const auto& e:entries)if(e.enabled&&e.function&&(e.kind==Render||e.kind==Present)){requested=true;break;}
    if(!requested)return;
    const auto ceiling=next_serial;int32_t width=0,height=0;
    Natives::GRAPHICS::GET_SCREEN_RESOLUTION(&width,&height);
    if(getNativeCallStatus()!=NativeCall_Ok||width<=0||height<=0)return;
    for(auto& e:entries)if(e.enabled&&e.function&&e.serial<ceiling&&e.born!=frame&&(e.kind==Render||e.kind==Present)){
        Call call{};call.snapshot=e;call.render={sizeof(ShvNxRenderFrame),frame,uint32_t(width),uint32_t(height),sh_get_tick()};dispatch(e,call);
    }
}
}
