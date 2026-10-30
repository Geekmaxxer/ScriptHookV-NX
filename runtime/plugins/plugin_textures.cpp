#include "plugin_textures.hpp"
#include "plugin_loader.hpp"
#include "../core/engine_api.hpp"
#include "../core/storage.hpp"
#include "natives.h"

namespace {
struct Allocation {uintptr_t address;size_t bytes;};
Allocation allocations[64];
void* image_allocate(size_t bytes){
    if(!bytes||bytes>8*1024*1024)return nullptr;
    bytes=(bytes+4095)&~size_t(4095);
    Allocation* slot=nullptr;for(auto& a:allocations)if(!a.address){slot=&a;break;}
    if(!slot||!ScriptHook::PluginLoader::ReserveExtraMemory(bytes))return nullptr;
    uintptr_t address=0;
    if(ScriptHook::sdk_call<uint32_t(*)(uintptr_t*,size_t)>(binding::allocate_pages)(&address,bytes)||!address||address%4096){
        ScriptHook::PluginLoader::ReleaseExtraMemory(bytes);return nullptr;
    }
    *slot={address,bytes};return reinterpret_cast<void*>(address);
}
void image_free(void* pointer){
    if(!pointer)return;
    for(auto& a:allocations)if(a.address==reinterpret_cast<uintptr_t>(pointer)){
        if(ScriptHook::sdk_call<uint32_t(*)(uintptr_t,size_t)>(binding::free_pages)(a.address,a.bytes))return;
        ScriptHook::sdk_call<void(*)(uintptr_t)>(binding::free_address)(a.address);
        ScriptHook::PluginLoader::ReleaseExtraMemory(a.bytes);a={};return;
    }
}
void* image_reallocate(void* pointer,size_t bytes){
    if(!pointer)return image_allocate(bytes);
    for(const auto& a:allocations)if(a.address==reinterpret_cast<uintptr_t>(pointer)){
        if(bytes<=a.bytes)return pointer;
        auto* next=image_allocate(bytes);if(!next)return nullptr;
        auto* out=static_cast<uint8_t*>(next);auto* in=static_cast<const uint8_t*>(pointer);
        for(size_t i=0;i<a.bytes;++i)out[i]=in[i];
        image_free(pointer);return next;
    }
    return nullptr;
}
}
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_NO_THREAD_LOCALS
#define STBI_MAX_DIMENSIONS 1024
#define STBI_ASSERT(x) ((void)0)
#define STBI_MALLOC(n) image_allocate(n)
#define STBI_REALLOC(p,n) image_reallocate(p,n)
#define STBI_FREE(p) image_free(p)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../third_party/stb_image.h"
#pragma GCC diagnostic pop

namespace ScriptHook {
namespace {
constexpr size_t max_textures=64,max_instances=128;
struct Texture {void* owner;uintptr_t texture;size_t bytes;char name[64],id[32];};
struct Instance {void* owner;uint64_t expires;ShvNxTextureDraw draw;bool one_frame;};
Texture textures[max_textures];Instance instances[max_instances];bool loading;size_t active_instances;
bool same(const char* a,const char* b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
uint32_t big32(const uint8_t* p){return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];}
bool png(const uint8_t* data,size_t bytes){
    constexpr uint8_t signature[]={137,80,78,71,13,10,26,10};
    if(bytes<33)return false;
    for(size_t i=0;i<8;++i)if(data[i]!=signature[i])return false;
    size_t at=8;bool first=true;
    while(at+12<=bytes){
        const auto n=big32(data+at);if(n>bytes-at-12)return false;
        if(first&&(n!=13||big32(data+at+4)!=0x49484452))return false;
        if(first&&(!big32(data+at+8)||!big32(data+at+12)||big32(data+at+8)>1024||big32(data+at+12)>1024))return false;
        first=false;uint32_t crc=~0u;
        for(size_t i=at+4;i<at+8+n;++i){crc^=data[i];for(unsigned b=0;b<8;++b)crc=(crc>>1)^(0xedb88320u&uint32_t(0-int32_t(crc&1)));}
        if(~crc!=big32(data+at+8+n))return false;
        const bool end=big32(data+at+4)==0x49454e44;
        at+=12+n;if(end)return n==0&&at==bytes;
    }
    return false;
}
int colour(float n){return n<=0.f?0:n>=1.f?255:int(n*255.f+.5f);}
float sine_turns(float turns){
    float t=turns-float(int(turns));
    if(t>.5f)t-=1.f;
    if(t<-.5f)t+=1.f;
    if(t>.25f)t=.5f-t;
    if(t<-.25f)t=-.5f-t;
    const float x=t*6.283185307179586f,z=x*x;
    return x*(1.f-z*(1.f/6.f-z*(1.f/120.f-z*(1.f/5040.f-z*(1.f/362880.f-z/39916800.f)))));
}
}
int PluginTextures::Create(void* owner,const char* id,const char* name){
    if(!owner||!id||!name||loading||!EngineApi::Context())return 0;
    Texture* free=nullptr;size_t owned=0;
    for(auto& t:textures){if(t.owner==owner){++owned;if(same(t.id,id)&&same(t.name,name))return int(&t-textures)+1;}if(!t.owner&&!free)free=&t;}
    if(!free||owned>=16)return 0;
    struct Busy {Busy(){loading=true;}~Busy(){loading=false;}} busy;
    char path[128]{};size_t at=0;
    for(const char* s="assets/";*s;++s)path[at++]=*s;
    for(const char* s=id;*s&&at+1<sizeof(path);++s)path[at++]=*s;
    path[at++]='/';for(const char* s=name;*s&&at+1<sizeof(path);++s)path[at++]=*s;
    size_t bytes=0;uint8_t* file=nullptr;
    {Storage disk;
        if(!disk.Size(path,bytes)||bytes<33||bytes>4*1024*1024)return 0;
        file=static_cast<uint8_t*>(image_allocate(bytes));if(!file)return 0;
        size_t got=0;
        if(!disk.Read(path,file,bytes,got)||got!=bytes||!png(file,bytes)){image_free(file);return 0;}
    } // Decode/upload after closing the filesystem session.
    int width=0,height=0,channels=0;
    auto* pixels=stbi_load_from_memory(file,int(bytes),&width,&height,&channels,4);image_free(file);
    if(!pixels)return 0;
    const size_t size=size_t(width)*size_t(height)*4;
    const size_t charge=2*((size+4095)&~size_t(4095))+4096;
    if(!PluginLoader::ReserveExtraMemory(charge)){image_free(pixels);return 0;}
    for(size_t i=0;i<size;i+=4){const auto r=pixels[i];pixels[i]=pixels[i+2];pixels[i+2]=r;}
    const auto texture=EngineApi::CreateTexture(pixels,uint32_t(width),uint32_t(height));image_free(pixels);
    if(!texture){PluginLoader::ReleaseExtraMemory(charge);return 0;}
    *free={};free->owner=owner;free->texture=texture;free->bytes=charge;
    for(size_t i=0;name[i]&&i+1<sizeof(free->name);++i)free->name[i]=name[i];
    for(size_t i=0;id[i]&&i+1<sizeof(free->id);++i)free->id[i]=id[i];
    return int(free-textures)+1;
}
bool PluginTextures::Draw(void* owner,const ShvNxTextureDraw* pointer){
    ShvNxTextureDraw d{};
    if(!EngineApi::Context()||!Memory::Read(reinterpret_cast<uintptr_t>(pointer),d)||d.id<=0||size_t(d.id)>max_textures||
       textures[d.id-1].owner!=owner||d.index<0||d.index>=64||d.time<0)return false;
    const float values[]={d.sizeX,d.sizeY,d.centerX,d.centerY,d.posX,d.posY,d.rotation,d.heightScale,d.r,d.g,d.b,d.a};
    for(float value:values)if(!__builtin_isfinite(value)||value<-4096.f||value>4096.f)return false;
    if(d.sizeX<=0.f||d.sizeY<=0.f||d.heightScale<=0.f)return false;
    Instance* free=nullptr;
    for(auto& i:instances){if(i.owner==owner&&i.draw.id==d.id&&i.draw.index==d.index){free=&i;break;}if(!i.owner&&!free)free=&i;}
    if(!free)return false;
    if(!free->owner)++active_instances;
    *free={owner,sh_get_tick()+uint64_t(d.time)*19200,d,d.time==0};return true;
}
void PluginTextures::Stop(void* owner){if(!owner)return;for(auto& i:instances)if(i.owner==owner){i={};--active_instances;}}
bool PluginTextures::Active(void* owner){if(!active_instances)return false;if(!owner)return true;for(const auto& i:instances)if(i.owner==owner)return true;return false;}
size_t PluginTextures::Bytes(void* owner){size_t bytes=0;for(const auto& t:textures)if(t.owner==owner)bytes+=t.bytes;return bytes;}
void PluginTextures::Frame(){
    if(!active_instances)return;
    const auto now=sh_get_tick();Instance* ordered[max_instances]{};size_t count=0;
    for(auto& i:instances)if(i.owner){
        if(!i.one_frame&&now>i.expires){i={};--active_instances;continue;}
        size_t at=count++;
        while(at&&ordered[at-1]->draw.level>i.draw.level){ordered[at]=ordered[at-1];--at;}
        ordered[at]=&i;
    }
    int32_t screen_x=0,screen_y=0;Natives::GRAPHICS::GET_SCREEN_RESOLUTION(&screen_x,&screen_y);
    const float aspect=getNativeCallStatus()==NativeCall_Ok&&screen_x>0&&screen_y>0?float(screen_x)/float(screen_y):1.f;
    for(size_t n=0;n<count;++n){auto& i=*ordered[n];const auto& d=i.draw;
        float dx=(.5f-d.centerX)*d.sizeX,dy=(.5f-d.centerY)*d.sizeY*d.heightScale;
        if(d.rotation!=0.f){
            const float sine=sine_turns(d.rotation),cosine=sine_turns(d.rotation+.25f);
            const auto x=dx;dx=x*cosine-dy*sine/aspect;dy=x*sine*aspect+dy*cosine;
        }
        (void)EngineApi::Sprite(textures[d.id-1].texture,d.posX+dx,d.posY+dy,d.sizeX,d.sizeY*d.heightScale,d.rotation*360.f,colour(d.r),colour(d.g),colour(d.b),colour(d.a));
        if(i.one_frame){i={};--active_instances;}
    }
}
}
