#include "plugin.h"
#include "extensions.h"
#include "plugin_memory.h"
#include "plugin_files.h"
#include "plugin_catalog.h"
#include "plugin_file_mutations.h"
#include "plugin_resources.h"
namespace { const ShvNxApi* api; const ShvNxExtensions* extensions; const ShvNxMemory* memory; const ShvNxFiles* files; const ShvNxCatalog* catalog; const ShvNxFileMutations* mutations; const ShvNxResources* resources; }
extern "C" {
SHV_NX_API bool shvPluginBindResources(const ShvNxResources* value) {
    if (!value || value->abi != SHV_NX_RESOURCES_ABI_VERSION || value->size < sizeof(ShvNxResources) ||
        !value->register_resource || !value->release || !value->forget || !value->active || !value->pinned || !value->count) return false;
    resources = value;
    return true;
}
bool scriptResourcesAvailable() { return api && resources; }
ShvNxResourceHandle scriptResourceRegister(ShvNxResourceCleanup cleanup, void* context) {
    return api && resources ? resources->register_resource(api->plugin, cleanup, context) : 0;
}
bool scriptResourceRelease(ShvNxResourceHandle handle) { return api && resources && resources->release(api->plugin, handle); }
bool scriptResourceForget(ShvNxResourceHandle handle) { return api && resources && resources->forget(api->plugin, handle); }
bool scriptResourceActive(ShvNxResourceHandle handle) { return api && resources && resources->active(api->plugin, handle); }
bool scriptResourcePinned(const void* pointer) { return api && resources && resources->pinned(api->plugin, pointer); }
uint32_t scriptResourceCount() { return api && resources ? resources->count(api->plugin) : 0; }
SHV_NX_API bool shvPluginBindFileMutations(const ShvNxFileMutations* value) {
    if (!value || value->abi != SHV_NX_FILE_MUTATIONS_ABI_VERSION || value->size < sizeof(ShvNxFileMutations) ||
        !value->rename_file || !value->delete_file) return false;
    mutations = value;
    return true;
}
bool scriptFileMutationsAvailable() { return api && mutations; }
ShvNxFileStatus scriptFileRename(const char* from, const char* to) {
    return api && mutations ? mutations->rename_file(api->plugin, from, to) : ShvNxFile_Unavailable;
}
ShvNxFileStatus scriptFileDelete(const char* path) {
    return api && mutations ? mutations->delete_file(api->plugin, path) : ShvNxFile_Unavailable;
}
SHV_NX_API bool shvPluginBindCatalog(const ShvNxCatalog* value) {
    if (!value || value->abi != SHV_NX_CATALOG_ABI_VERSION || value->size < sizeof(ShvNxCatalog) || !value->list)
        return false;
    catalog = value;
    return true;
}
bool scriptCatalogAvailable() { return api && catalog; }
ShvNxFileStatus scriptDirectoryList(const char* path, uint32_t offset,
    ShvNxDirectoryEntry* entries, uint32_t capacity, ShvNxDirectoryPage* page) {
    return api && catalog ? catalog->list(api->plugin, path, offset, entries, capacity, page) : ShvNxFile_Unavailable;
}
SHV_NX_API bool shvPluginBindFiles(const ShvNxFiles* value) {
    if (!value || value->abi != SHV_NX_FILES_ABI_VERSION || value->size < sizeof(ShvNxFiles) ||
        !value->size_of || !value->read_at || !value->write_at || !value->create || !value->directory) return false;
    files = value;
    return true;
}
bool scriptFilesAvailable() { return api && files; }
ShvNxFileStatus scriptFileSize(const char* path, uint64_t* bytes) {
    return api && files ? files->size_of(api->plugin, path, bytes) : ShvNxFile_Unavailable;
}
ShvNxFileStatus scriptFileReadAt(const char* path, uint64_t offset, void* out, size_t capacity, size_t* bytes) {
    return api && files ? files->read_at(api->plugin, path, offset, out, capacity, bytes) : ShvNxFile_Unavailable;
}
ShvNxFileStatus scriptFileWriteAt(const char* path, uint64_t offset, const void* data, size_t bytes) {
    return api && files ? files->write_at(api->plugin, path, offset, data, bytes) : ShvNxFile_Unavailable;
}
ShvNxFileStatus scriptFileCreate(const char* path, uint64_t bytes) {
    return api && files ? files->create(api->plugin, path, bytes) : ShvNxFile_Unavailable;
}
ShvNxFileStatus scriptDirectoryCreate(const char* path) {
    return api && files ? files->directory(api->plugin, path) : ShvNxFile_Unavailable;
}
SHV_NX_API bool shvPluginBindMemory(const ShvNxMemory* value){
    if(!value||value->abi!=SHV_NX_MEMORY_ABI_VERSION||value->size<sizeof(ShvNxMemory)||!value->allocate||!value->reallocate||!value->free||!value->allocation_size||!value->stats||!value->trim)return false;
    memory=value;return true;
}
bool scriptMemoryAvailable(){return api&&memory;}
void* scriptAllocate(size_t bytes,size_t alignment,uint32_t flags){return api&&memory?memory->allocate(api->plugin,bytes,alignment,flags):nullptr;}
void* scriptCalloc(size_t count,size_t bytes){return !bytes||count>SIZE_MAX/bytes?nullptr:scriptAllocate(count*bytes,16,SHV_NX_MEMORY_ZERO);}
void* scriptReallocate(void* p,size_t bytes){return api&&memory?memory->reallocate(api->plugin,p,bytes):nullptr;}
bool scriptFree(void* p){return api&&memory&&memory->free(api->plugin,p);}
size_t scriptAllocationSize(const void* p){return api&&memory?memory->allocation_size(api->plugin,p):0;}
bool scriptGetMemoryStats(ShvNxMemoryStats* s){return api&&memory&&memory->stats(api->plugin,s);}
bool scriptTrimMemory(){return api&&memory&&memory->trim(api->plugin);}
SHV_NX_API bool shvPluginBindExtensions(const ShvNxExtensions* value) {
    if(!value||value->abi!=SHV_NX_EXTENSIONS_ABI_VERSION||value->size<sizeof(ShvNxExtensions)||
       !value->register_script||!value->unregister_script||!value->enumerate||!value->global||!value->entity||
       !value->callback||!value->create_texture||!value->draw_texture)return false;
    extensions=value;return true;
}
bool shvBindApi(const ShvNxApi* value) {
#if defined(SHVNX_PLUGIN_C_RUNTIME) && SHVNX_PLUGIN_C_RUNTIME
    if(!memory)return false;
#endif
    if(!value||value->size!=sizeof(ShvNxApi)||value->abi!=SHV_NX_PLUGIN_ABI_VERSION||value->caller_abi!=SHV_NX_CALLER_ABI_VERSION||value->runtime_abi!=SHV_NX_RUNTIME_ABI_VERSION)return false;
    api=value;return true;
}
bool nativeInvokeContext(uint64_t h,void* c,uint32_t n){return api&&api->invoke(h,c,n);}
NativeCallStatus getNativeCallStatus(){return api?api->native_status():NativeCall_NotInitialized;}
uint64_t getLastNativeHash(){return api?api->native_hash():0;}
bool nativeIsAvailable(uint64_t h){return api&&api->native_available(h);}
void scriptWait(uint32_t ms){if(api)api->wait(ms);}
bool scriptRegisterAdditionalThread(ScriptFunction fn){return api&&extensions&&extensions->register_script(api->plugin,fn);}
bool scriptUnregisterChecked(ScriptFunction fn){return api&&extensions&&extensions->unregister_script(api->plugin,fn);}
void scriptRegister(ScriptFunction fn){(void)scriptRegisterAdditionalThread(fn);}
bool scriptRegisterChecked(ScriptFunction fn){return scriptRegisterAdditionalThread(fn);}
void scriptUnregister(ScriptFunction fn){(void)scriptUnregisterChecked(fn);}
void scriptUnregisterAll(){if(api&&extensions)(void)extensions->unregister_script(api->plugin,nullptr);}
uint64_t getScriptHookCapabilities(){return extensions?extensions->capabilities:0;}
int worldGetAllPeds(int* out,int n){return api&&extensions&&n>0?extensions->enumerate(api->plugin,0,out,uint32_t(n)):0;}
int worldGetAllVehicles(int* out,int n){return api&&extensions&&n>0?extensions->enumerate(api->plugin,1,out,uint32_t(n)):0;}
int worldGetAllObjects(int* out,int n){return api&&extensions&&n>0?extensions->enumerate(api->plugin,2,out,uint32_t(n)):0;}
int worldGetAllPickups(int* out,int n){return api&&extensions&&n>0?extensions->enumerate(api->plugin,3,out,uint32_t(n)):0;}
uint64_t* getGlobalPtr(int i){return api&&extensions&&i>=0?extensions->global(api->plugin,uint32_t(i)):nullptr;}
BYTE* getScriptHandleBaseAddress(int h){return api&&extensions?reinterpret_cast<BYTE*>(extensions->entity(api->plugin,h)):nullptr;}
bool controlHandlerRegister(ControlHandler f,uint32_t g,uint32_t c){return api&&extensions&&extensions->callback(api->plugin,0,reinterpret_cast<uintptr_t>(f),g,c,true);}
bool controlHandlerUnregister(ControlHandler f,uint32_t g,uint32_t c){return api&&extensions&&extensions->callback(api->plugin,0,reinterpret_cast<uintptr_t>(f),g,c,false);}
bool renderCallbackRegister(RenderCallback f){return api&&extensions&&extensions->callback(api->plugin,1,reinterpret_cast<uintptr_t>(f),0,0,true);}
bool renderCallbackUnregister(RenderCallback f){return api&&extensions&&extensions->callback(api->plugin,1,reinterpret_cast<uintptr_t>(f),0,0,false);}
bool presentCallbackRegisterChecked(PresentCallback f){return api&&extensions&&extensions->callback(api->plugin,2,reinterpret_cast<uintptr_t>(f),0,0,true);}
void presentCallbackRegister(PresentCallback f){(void)presentCallbackRegisterChecked(f);}
void presentCallbackUnregister(PresentCallback f){if(api&&extensions)(void)extensions->callback(api->plugin,2,reinterpret_cast<uintptr_t>(f),0,0,false);}
bool keyboardHandlerRegisterChecked(KeyboardHandler f){return api&&extensions&&extensions->callback(api->plugin,3,reinterpret_cast<uintptr_t>(f),0,0,true);}
void keyboardHandlerRegister(KeyboardHandler f){(void)keyboardHandlerRegisterChecked(f);}
void keyboardHandlerUnregister(KeyboardHandler f){if(api&&extensions)(void)extensions->callback(api->plugin,3,reinterpret_cast<uintptr_t>(f),0,0,false);}
int createTexture(const char* file){return api&&extensions?extensions->create_texture(api->plugin,file):0;}
bool drawTextureChecked(const ShvNxTextureDraw* d){return api&&extensions&&extensions->draw_texture(api->plugin,d);}
void drawTexture(int id,int index,int level,int time,float sx,float sy,float cx,float cy,float x,float y,float rotation,float scale,float r,float g,float b,float a){
    const ShvNxTextureDraw d={id,index,level,time,sx,sy,cx,cy,x,y,rotation,scale,r,g,b,a};(void)drawTextureChecked(&d);
}
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
