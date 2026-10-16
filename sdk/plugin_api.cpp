#include "plugin.h"
namespace { const ShvNxApi* api; }
extern "C" {
bool shvBindApi(const ShvNxApi* value) {
    if(!value||value->size!=sizeof(ShvNxApi)||value->abi!=SHV_NX_PLUGIN_ABI_VERSION||value->caller_abi!=SHV_NX_CALLER_ABI_VERSION||value->runtime_abi!=SHV_NX_RUNTIME_ABI_VERSION)return false;
    api=value;return true;
}
bool nativeInvokeContext(uint64_t h,void* c,uint32_t n){return api&&api->invoke(h,c,n);}
NativeCallStatus getNativeCallStatus(){return api?api->native_status():NativeCall_NotInitialized;}
uint64_t getLastNativeHash(){return api?api->native_hash():0;}
bool nativeIsAvailable(uint64_t h){return api&&api->native_available(h);}
void scriptWait(uint32_t ms){if(api)api->wait(ms);}
bool registerCustomNative(uint64_t h,NativeHandler fn){return api&&api->register_native(api->plugin,h,fn);}
bool scriptSaveFile(const char* path,const void* data,size_t bytes){return api&&api->write_file(api->plugin,path,data,bytes);}
bool scriptReadFile(const char* path,void* data,size_t bytes,size_t* got){return api&&api->read_file(api->plugin,path,data,bytes,got);}
void nativeInit(uint64_t h){if(api)api->raw_init(h);}
void nativePush64(uint64_t v){if(api)api->raw_push(v);}
uint64_t* nativeCall(){static uint64_t empty[3];return api?api->raw_call():empty;}
void* nativeGetCallContext(){return api?api->raw_context():nullptr;}
uint64_t* nativeCallChecked(uint64_t h){static uint64_t empty[3];return api?api->raw_checked(h):empty;}
NativeHandler getNativeHandler(uint64_t h){return api?api->handler(h):nullptr;}
uint32_t getNativeCallerAbiVersion(){return api?api->caller_abi:0;}
uint32_t getScriptRuntimeAbiVersion(){return api?api->runtime_abi:0;}
void* memset(void* p,int value,size_t n){auto* out=static_cast<unsigned char*>(p);for(size_t i=0;i<n;++i)out[i]=value;return p;}
void* memcpy(void* p,const void* q,size_t n){auto* out=static_cast<unsigned char*>(p);auto* in=static_cast<const unsigned char*>(q);for(size_t i=0;i<n;++i)out[i]=in[i];return p;}
}
