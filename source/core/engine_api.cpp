#include "engine_api.hpp"
#include "bindings.hpp"
#include "engine_bindings.hpp"
#include "storage.hpp"

namespace ScriptHook {
namespace {
constexpr uint32_t max_pool=65536;
struct Pool { uintptr_t data,flags;uint32_t count,stride;int32_t free_head; };
bool pool(uintptr_t address,Pool& out) {
    return Memory::Read(address,out)&&out.count<=max_pool&&out.stride>=16&&out.stride<=65536&&
        out.free_head>=-1&&(out.free_head<0||uint32_t(out.free_head)<out.count)&&
        (!out.count||(Memory::IsReadable(out.data,size_t(out.count)*out.stride)&&Memory::IsReadable(out.flags,out.count)));
}
bool guid_pool(Pool& out) {
    uintptr_t address=0;
    return Memory::Read(Memory::s_mainBase+engine_binding::guid_pool,address)&&pool(address,out);
}
uintptr_t from_guid(const Pool& p,int32_t handle) {
    if(handle<=0)return 0;
    const uint32_t index=uint32_t(handle)>>8;
    if(index>=p.count)return 0;
    const uint8_t flag=*reinterpret_cast<const uint8_t*>(p.flags+index);
    if((flag&0x80)||flag!=uint8_t(handle))return 0;
    uintptr_t entity=0;
    if(!Memory::Read(p.data+size_t(index)*p.stride+8,entity)||!Memory::IsReadable(entity,0x30))return 0;
    return entity;
}
int32_t guid_for(uintptr_t entity,Pool& p) {
    uintptr_t node=0;
    if(!Memory::Read(entity+0x10,node))return 0;
    // Use existing GUIDs even when the GUID pool is full. Never call the game
    // allocator with an exhausted freelist: the stock constructor assumes room.
    for(uint32_t depth=0;node&&depth<256;++depth){
        uintptr_t extension=0,next=0,vtable=0;
        if(!Memory::Read(node,extension)||!Memory::Read(node+8,next))return 0;
        if(extension&&Memory::Read(extension,vtable)&&vtable==Memory::s_mainBase+engine_binding::guid_vtable){
            if(extension<p.data||(extension-p.data)%p.stride)return 0;
            const auto index=(extension-p.data)/p.stride;
            if(index>=p.count)return 0;
            const auto handle=int32_t((index<<8)|*reinterpret_cast<const uint8_t*>(p.flags+index));
            return from_guid(p,handle)==entity?handle:0;
        }
        node=next;
        if(node&&depth==255)return 0;
    }
    uintptr_t address=0;int32_t free_head=-1;
    if(!Memory::Read(Memory::s_mainBase+engine_binding::guid_pool,address)||!Memory::Read(address+0x18,free_head)||
        free_head<0||uint32_t(free_head)>=p.count)return 0;
    const auto handle=reinterpret_cast<int32_t(*)(uintptr_t)>(Memory::s_mainBase+engine_binding::create_guid)(entity);
    return from_guid(p,handle)==entity?handle:0;
}
}
bool EngineApi::Verified() {
    if(!Memory::s_mainBase)return false;
    static uintptr_t checked_base=0;static bool valid=false;
    if(checked_base==Memory::s_mainBase)return valid;
    checked_base=Memory::s_mainBase;valid=false;
    for(const auto& proof:engine_binding::proofs){
        uint8_t bytes[16]{};
        if(!Memory::Read(checked_base+proof.offset,bytes))return false;
        for(unsigned i=0;i<16;++i)if(bytes[i]!=proof.bytes[i])return false;
    }
    valid=true;return true;
}
bool EngineApi::Context() {
    uintptr_t thread=0,active=0,aux=0;uint8_t enabled=0;uint32_t id=0,state=0;const auto tls=sh_get_thread_pointer();
    return Verified()&&Memory::Read(tls+binding::script_tls_active,enabled)&&enabled==1&&
        Memory::Read(tls+binding::script_tls_thread,thread)&&Memory::IsReadable(thread,0x20)&&
        Memory::Read(Memory::s_mainBase+binding::active_script_thread,active)&&active==thread&&
        Memory::Read(thread+8,id)&&id&&Memory::Read(thread+0x10,state)&&state<=1&&
        Memory::Read(Memory::s_mainBase+binding::active_script_aux,aux)&&aux==thread+0xd4;
}
uintptr_t EngineApi::EntityAddress(int32_t handle) {
    Pool p{};return Context()&&guid_pool(p)?from_guid(p,handle):0;
}
uint64_t* EngineApi::Global(uint32_t index) {
    if(!Context()||index>=0x1000000)return nullptr;
    const auto bank=index>>18,slot=index&0x3ffff;uintptr_t block=0;uint32_t size=0;
    if(!Memory::Read(Memory::s_mainBase+engine_binding::global_sizes+bank*4,size)||size>0x3ffff||slot>=size||
       !Memory::Read(Memory::s_mainBase+engine_binding::global_blocks+bank*8,block)||
       !Memory::IsWritable(block,size_t(size)*8))return nullptr;
    return reinterpret_cast<uint64_t*>(block+size_t(slot)*8);
}
int EngineApi::Enumerate(uint32_t kind,int32_t* out,uint32_t capacity) {
    if(!Context()||kind>3||!capacity||capacity>max_pool||!Memory::IsWritable(reinterpret_cast<uintptr_t>(out),size_t(capacity)*4))return 0;
    Pool guids{};if(!guid_pool(guids))return 0;
    uintptr_t address=0;
    constexpr uint32_t offsets[]={engine_binding::ped_pool,engine_binding::vehicle_pool,engine_binding::object_pool,engine_binding::pickup_pool};
    if(!Memory::Read(Memory::s_mainBase+offsets[kind],address))return 0;
    uint32_t total=0,written=0;uintptr_t data=0,flags=0;uint64_t stride=0;Pool plain{};
    if(kind==1){
        if(!Memory::Read(address,address)||!Memory::Read(address,data)||!Memory::Read(address+8,total)||total>max_pool||
           !Memory::Read(address+0x18,stride)||stride<8||stride>65536||!Memory::Read(address+0x30,flags)||
           (total&&(!Memory::IsReadable(data,size_t(total)*stride)||!Memory::IsReadable(flags,((size_t(total)+31)/32)*4))))return 0;
    }else{
        if(!pool(address,plain))return 0;
        data=plain.data;flags=plain.flags;total=plain.count;stride=plain.stride;
    }
    for(uint32_t i=0;i<total&&written<capacity;++i){
        uintptr_t entity=data+size_t(i)*stride;
        if(kind==1){
            const auto bits=*reinterpret_cast<const uint32_t*>(flags+size_t(i>>5)*4);
            if(!(bits&(1u<<(i&31)))||!Memory::Read(entity,entity))continue;
        }else if(*reinterpret_cast<const uint8_t*>(flags+i)&0x80)continue;
        if(!Memory::IsReadable(entity,0x30))continue;
        // Source and target agree: fwEntity type 4=ped, 3=vehicle, 5=object.
        const uint8_t type=*reinterpret_cast<const uint8_t*>(entity+0x28);
        if(type!=(kind==0?4:kind==1?3:5))continue;
        const auto handle=guid_for(entity,guids);
        if(handle>0)out[written++]=handle;
    }
    return int(written);
}
bool EngineApi::KeyboardSupported() {
    if(!Memory::ResolveSdk())return false;
    for(const auto& proof:engine_binding::sdk_proofs){
        uint8_t bytes[16]{};
        if(!Memory::Read(Memory::s_sdkBase+proof.offset,bytes))return false;
        for(unsigned i=0;i<16;++i)if(bytes[i]!=proof.bytes[i])return false;
    }
    return true;
}
bool EngineApi::Keyboard(KeyboardState& out) {
    if(!Context()||!KeyboardSupported())return false;
    static bool initialized=false;
    if(!initialized){
        if(sdk_call<uint32_t(*)()>(engine_binding::keyboard_initialize)())return false;
        initialized=true;
    }
    int32_t count=0;out={};
    return !sdk_call<uint32_t(*)(int32_t*,KeyboardState*,int32_t)>(engine_binding::keyboard_states)(&count,&out,1)&&count>=0&&count<=1;
}
uintptr_t EngineApi::CreateTexture(const void* bgra,uint32_t width,uint32_t height) {
    if(!Context()||!width||!height||width>1024||height>1024||!Memory::IsReadable(reinterpret_cast<uintptr_t>(bgra),size_t(width)*height*4))return 0;
    uintptr_t factory=0,vtable=0,create=0;
    if(!Memory::Read(Memory::s_mainBase+engine_binding::texture_factory,factory)||!Memory::Read(factory,vtable)||
       vtable!=Memory::s_mainBase+engine_binding::texture_factory_vtable||!Memory::Read(vtable+0x20,create)||
       create!=Memory::s_mainBase+engine_binding::texture_create)return 0;
    // Actual SGA vtable slot: this,width,height,grcTextureFormat,pixels,mips,params.
    // Format 2 is the game's uncompressed A8R8G8B8 (BGRA byte order).
    const auto texture=reinterpret_cast<uintptr_t(*)(uintptr_t,uint32_t,uint32_t,uint32_t,const void*,uint32_t,const void*)>(create)(factory,width,height,2,bgra,1,nullptr);
    uintptr_t table=0,valid=0;
    if(!Memory::Read(texture,table)||table!=Memory::s_mainBase+engine_binding::texture_vtable||
       !Memory::Read(table+0x60,valid)||valid!=Memory::s_mainBase+engine_binding::texture_valid)return 0;
    if(reinterpret_cast<bool(*)(uintptr_t)>(valid)(texture))return texture;
    uintptr_t destroy=0;
    if(Memory::Read(table+8,destroy)&&destroy==Memory::s_mainBase+engine_binding::texture_delete)
        reinterpret_cast<void(*)(uintptr_t)>(destroy)(texture);
    return 0;
}
bool EngineApi::Sprite(uintptr_t texture,float x,float y,float width,float height,float degrees,int r,int g,int b,int a) {
    if(!Context()||!Memory::IsReadable(texture,0x88))return false;
    int32_t buffer=-1;uint32_t count=0;
    if(!Memory::Read(Memory::s_mainBase+engine_binding::rectangle_buffer,buffer)||buffer<0||buffer>2||
       !Memory::Read(Memory::s_mainBase+engine_binding::rectangles+size_t(buffer)*0x9c48+0x9c40,count)||count>=500)return false;
    const float uv0[2]={0.f,0.f},uv1[2]={1.f,1.f};
    // The target passes Vector2 UV objects indirectly, not as an AAPCS HFA.
    using Draw=void(*)(uintptr_t,float,float,float,float,float,int,int,int,int,uint32_t,uint32_t,const float*,const float*,bool,bool);
    reinterpret_cast<Draw>(Memory::s_mainBase+engine_binding::draw_sprite)(texture,x,y,width,height,degrees,r,g,b,a,0,14,uv0,uv1,false,false);
    return true;
}
}
