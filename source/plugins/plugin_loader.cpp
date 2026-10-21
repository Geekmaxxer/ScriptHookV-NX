#include "plugin_loader.hpp"
#include "../core/runtime_config.hpp"
#include "plugin.h"
#include "../../include/nativeCaller.h"
#include "../core/storage.hpp"
#include "../script/native_table.hpp"
#include "../script/native_manifest.hpp"
#include "../script/script_manager.hpp"
namespace ScriptHook {
namespace {
constexpr size_t max_plugins=16,max_image=2*1024*1024;
static_assert(SHVNX_MAX_RESIDENT_PLUGINS<=max_plugins);
enum Policy : uint32_t { Disabled, Manual, Auto };
struct Nro {uint32_t start,mod,reserved[2],magic,version,size,flags,text,text_bytes,ro,ro_bytes,data,data_bytes,bss,reserved2;uint8_t rest[64];};
static_assert(sizeof(Nro)==0x80);
struct alignas(8) Plugin {
    uint8_t module[0x140],registration[0x20];
    uintptr_t image,bss,nrr,base,tls;size_t image_bytes,bss_bytes,text_bytes;
    ShvNxApi api;
    ScriptFunction run,cleanup;
    uint32_t cleanup_calls,cleanup_failures;
    char filename[64],canonical[55],id[32];
    CustomNativeEntry pending[32];uint32_t pending_count;
    uint32_t step,error;
    uint32_t policy,loaded_policy;
    bool loaded,registered,active,published,queued,present,conflict,kill_queued,stopped,stopping,cleanup_done;
};
struct Candidate { char filename[64],canonical[55]; uint32_t policy; bool conflict; };
Plugin plugins[max_plugins];size_t count;
Candidate candidates[max_plugins];size_t candidate_count;
DirectoryEntry directory[16];
alignas(8) uint8_t manifest_buffer[65536];
uint32_t stage,error,skipped,last_action_error;
uint32_t discovered,disabled;
size_t plugin_entries,binding_entries;
uint32_t last_frame;bool frame_seen,reported;
uintptr_t owner_tls;
bool rescan_requested,ro_ready;
uint32_t policy_slot=UINT32_MAX,policy_requested=UINT32_MAX,policy_previous=UINT32_MAX,policy_effective=UINT32_MAX;
uint32_t policy_rename_result=UINT32_MAX,policy_postcheck;
bool policy_rename_attempted;
uint32_t reported_running_mask;
bool basename(const char* s,size_t maximum,bool dot) {
    if(!s)return false;
    for(size_t i=0;i<maximum;++i){char c=0;if(!Memory::Read(reinterpret_cast<uintptr_t>(s)+i,c))return false;if(!c)return i>0;
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||(dot&&i&&c=='.'&&s[i-1]!='.')))return false;}
    return false;
}
void copy(char* to,const char* from){while((*to++=*from++)){};}
bool same_id(const char* a,const char* b){
    for(;;){char left=*a++,right=*b++;
        if(left>='A'&&left<='Z')left+=('a'-'A');
        if(right>='A'&&right<='Z')right+=('a'-'A');
        if(left!=right)return false;
        if(!left)return true;
    }
}
bool less(const char* a,const char* b){while(*a&&*a==*b){++a;++b;}return uint8_t(*a)<uint8_t(*b);}
bool suffix(const char* name,const char* ending){
    size_t n=0,m=0;while(name[n])++n;while(ending[m])++m;if(n<=m)return false;
    for(size_t i=0;i<m;++i){char c=name[n-m+i];if(c>='A'&&c<='Z')c+=('a'-'A');if(c!=ending[i])return false;}return true;
}
size_t length(const char* name){size_t n=0;while(name[n])++n;return n;}
bool parse_name(const char* name,char (&canonical)[55],uint32_t& policy){
    if(!basename(name,64,true))return false;
    policy=suffix(name,".nro.disabled")?Disabled:suffix(name,".nro.manual")?Manual:Auto;
    if(policy==Auto&&!suffix(name,".nro"))return false;
    const auto bytes=length(name)-(policy==Disabled?9:policy==Manual?7:0);
    if(bytes>=sizeof(canonical))return false;
    for(size_t i=0;i<bytes;++i)canonical[i]=name[i];
    canonical[bytes]=0;return true;
}
bool own_thread(){return owner_tls&&owner_tls==sh_get_thread_pointer();}
bool reject(uint32_t reason){last_action_error=reason;reported=false;return false;}
size_t resident_count(){size_t value=0;for(const auto& p:plugins)if(p.loaded)++value;return value;}
size_t memory_cost(const Plugin& p){return (p.image?2*p.image_bytes:0)+(p.bss?2*p.bss_bytes:0)+(p.nrr?4096:0);}
size_t memory_used(){size_t value=0;for(const auto& p:plugins)value+=memory_cost(p);return value;}
uint32_t running_mask(){
    uint32_t value=0;
    for(size_t i=0;i<count;++i)if(plugins[i].loaded&&plugins[i].active&&ScriptManager::Contains(plugins[i].run))value|=1u<<i;
    return value;
}
const char* error_name(uint32_t value,uint32_t step){
    switch(value){
        case 0:return "none";
        case 0x10001:return "invalid_file_size";
        case 0x10002:return "allocation_failed";
        case 0x10003:return "incomplete_file_read";
        case 0x10004:return "invalid_nro_header";
        case 0x10005:return "invalid_plugin_entry";
        case 0x10006:return "incompatible_plugin_descriptor";
        case 0x10007:return "duplicate_plugin_id";
        case 0x10008:return "plugin_initialization_failed";
        case 0x10009:return "script_capacity_exhausted";
        case 0x1000a:return "invalid_native_manifest";
        case 0x1000b:return "sdk_loader_unavailable";
        case 0x1000c:return "unsupported_implicit_initialization";
        case 0x1000e:return "custom_native_registration_failed";
        case 0x1000f:return "directory_entry_limit_exceeded";
        case 0x10010:return "invalid_directory_scan";
        case 0x10011:return "invalid_directory_read_result";
        case 0x10012:return "invalid_plugin_slot";
        case 0x10013:return "wrong_thread_or_loader_not_ready";
        case 0x10014:return "plugin_disabled_select_manual_or_auto";
        case 0x10015:return "resident_plugin_limit_reached";
        case 0x10016:return "plugin_memory_budget_exceeded";
        case 0x10017:return "duplicate_plugin_policy_files";
        case 0x10018:return "plugin_file_missing";
        case 0x10019:return "invalid_startup_policy";
        case 0x1001a:return "resident_mapping_retained_after_error";
        case 0x1001b:return "unfinished_native_call_in_plugin_startup";
        case 0x1001c:return "plugin_not_running";
        case 0x1001d:return "policy_rename_readback_failed";
        default:return step==2?"sdk_registration_failed":step==3?"sdk_module_load_failed":step==4?"sdk_symbol_lookup_failed":"filesystem_error";
    }
}
const char* step_name(uint32_t value){
    switch(value){case 0:return "not_started";case 1:return "read_and_validate";case 2:return "register_module";
        case 3:return "load_module";case 4:return "query_plugin";case 5:return "initialize_plugin";
        case 6:return "schedule_script";case 7:return "loaded";default:return "unknown";}
}
void filename(char (&out)[128],const char* prefix,const char* name){size_t n=0;while(*prefix)out[n++]=*prefix++;while(*name&&n+1<sizeof(out))out[n++]=*name++;out[n]=0;}
bool allocate(uintptr_t& address,size_t bytes){return !sdk_call<uint32_t(*)(uintptr_t*,size_t)>(binding::allocate_pages)(&address,bytes)&&address&&!(address%4096);}
void free_pages(uintptr_t& address,size_t bytes){if(!address)return;(void)sdk_call<uint32_t(*)(uintptr_t,size_t)>(binding::free_pages)(address,bytes);sdk_call<void(*)(uintptr_t)>(binding::free_address)(address);address=0;}
void discard(Plugin& p){
    // Published handlers and suspended fiber return addresses can point into
    // this image for the lifetime of the game. Never invalidate them.
    if(p.published||p.active)return;
    if(p.loaded){const auto rc=sdk_call<uint32_t(*)(void*)>(binding::ro_unload)(p.module);if(rc)return;p.loaded=false;}
    if(p.registered){sdk_call<void(*)(void*)>(binding::ro_unregister)(p.registration);p.registered=false;}
    free_pages(p.nrr,4096);free_pages(p.bss,p.bss_bytes);free_pages(p.image,p.image_bytes);
}
bool own(Plugin* p){for(auto& candidate:plugins)if(p==&candidate)return candidate.loaded&&candidate.tls==sh_get_thread_pointer();return false;}
bool code(Plugin& p,uintptr_t address){return address>=p.base+0x80&&address-p.base<p.text_bytes&&Memory::IsExecutable(address);}
bool register_native(void* token,uint64_t hash,NativeHandler handler){
    auto* p=static_cast<Plugin*>(token);
    if(!own(p)||!hash||!code(*p,reinterpret_cast<uintptr_t>(handler)))return false;
    if(p->active)return NativeTable::RegisterCustom(hash,handler);
    if(NativeTable::GetHandler(hash))return false;
    for(uint32_t i=0;i<p->pending_count;++i)if(p->pending[i].hash==hash)return p->pending[i].handler==handler;
    if(p->pending_count==32)return false;
    p->pending[p->pending_count++]={hash,handler};return true;
}
bool data_path(Plugin* p,const char* name,char (&out)[128]){
    if(!own(p)||!basename(name,64,true))return false;
    size_t n=0;const char* prefix="settings/";while(*prefix)out[n++]=*prefix++;
    for(const char* s=p->id;*s;++s)out[n++]=*s;
    out[n++]='-';for(const char* s=name;*s;++s)out[n++]=*s;out[n]=0;return true;
}
bool write_data(void* token,const char* name,const void* data,size_t bytes){char path[128]{};if(!data_path(static_cast<Plugin*>(token),name,path))return false;Storage disk;return disk.Write(path,data,bytes);}
bool read_data(void* token,const char* name,void* data,size_t bytes,size_t* got){char path[128]{};if(!Memory::IsWritable(reinterpret_cast<uintptr_t>(got),sizeof(*got))||!data_path(static_cast<Plugin*>(token),name,path))return false;Storage disk;return disk.Read(path,data,bytes,*got);}
void fail(Plugin& p,uint32_t reason){p.error=reason;discard(p);}
void load(Plugin& p){
    if(p.loaded){
        if(!p.active||!p.run){p.error=0x1001a;return;}
        if(ScriptManager::Contains(p.run))return;
        if(!ScriptManager::Register(p.run)){p.error=0x10009;return;}
        p.error=0;p.stopped=false;p.cleanup_done=false;p.step=7;return;
    }
    p.error=0;p.pending_count=0;
    if(resident_count()>=RuntimeConfig::Residents()){p.error=0x10015;return;}
    struct Reservation {
        uint32_t token=ScriptManager::ReserveSlot();
        ~Reservation(){ScriptManager::ReleaseReserved(token);}
    } reservation;
    if(!reservation.token){p.error=0x10009;return;}
    p.step=1;Nro h{};
    { Storage disk;if(!disk.mounted){p.error=disk.error;return;}
    char path[128]{};filename(path,"plugins/",p.filename);size_t size=0;
    if(!disk.Size(path,size)||size<0x1000||size>max_image||size%4096){p.error=disk.error?disk.error:0x10001;return;}
    if(memory_used()+2*(size+4096)+4096>RuntimeConfig::MemoryBytes()){p.error=0x10016;return;}
    p.image_bytes=size;
    if(!allocate(p.image,size)){p.error=0x10002;return;}
    size_t got=0;if(!disk.Read(path,reinterpret_cast<void*>(p.image),size,got)||got!=size){fail(p,disk.error?disk.error:0x10003);return;}
    h=*reinterpret_cast<const Nro*>(p.image);
    if(h.magic!=0x304f524e||h.version||h.flags||h.size!=size||h.text||h.text_bytes<0x1000||h.text_bytes%4096||
       h.ro!=h.text_bytes||h.ro_bytes%4096||uint64_t(h.ro)+h.ro_bytes!=h.data||h.data_bytes%4096||uint64_t(h.data)+h.data_bytes!=size||
       h.bss>max_image||h.bss%4096||h.mod<0x80||uint64_t(h.mod)+28>size||*reinterpret_cast<const uint32_t*>(p.image+h.mod)!=0x30444f4d){fail(p,0x10004);return;}
    } // Close/unmount the input file session before plugin initialization.
    const auto* raw=reinterpret_cast<const uint8_t*>(p.image);
    const int64_t dynamic=int64_t(h.mod)+*reinterpret_cast<const int32_t*>(raw+h.mod+4);
    if(dynamic<0||uint64_t(dynamic)>p.image_bytes-16){fail(p,0x1000c);return;}
    bool ended=false;
    for(size_t at=size_t(dynamic);at+16<=p.image_bytes;at+=16){
        const auto* tag=reinterpret_cast<const uint64_t*>(raw+at);
        if(!tag[0]){ended=true;break;}
        if((tag[0]==12||tag[0]==13||tag[0]==25||tag[0]==26||tag[0]==32)&&tag[1]){fail(p,0x1000c);return;}
    }
    if(!ended){fail(p,0x1000c);return;}
    p.bss_bytes=h.bss?h.bss:4096;p.text_bytes=h.text_bytes;
    if(memory_used()+2*p.bss_bytes+4096>RuntimeConfig::MemoryBytes()){fail(p,0x10016);return;}
    if(!allocate(p.bss,p.bss_bytes)||!allocate(p.nrr,4096)){fail(p,0x10002);return;}
    auto* nrr=reinterpret_cast<uint8_t*>(p.nrr);for(size_t i=0;i<4096;++i)nrr[i]=0;
    *reinterpret_cast<uint32_t*>(nrr)=0x3052524e;*reinterpret_cast<uint64_t*>(nrr+0x330)=0x0100b00b51230000ULL;
    *reinterpret_cast<uint32_t*>(nrr+0x338)=4096;*reinterpret_cast<uint32_t*>(nrr+0x340)=0x350;*reinterpret_cast<uint32_t*>(nrr+0x344)=1;
    sdk_call<void(*)(void*,size_t,const void*,size_t)>(binding::sha256)(nrr+0x350,32,reinterpret_cast<void*>(p.image),p.image_bytes);
    p.step=2;p.error=sdk_call<uint32_t(*)(void*,const void*)>(binding::ro_register)(p.registration,nrr);
    if(p.error){discard(p);return;}p.registered=true;
    p.step=3;p.error=sdk_call<uint32_t(*)(void*,const void*,void*,size_t,int)>(binding::ro_load)(p.module,reinterpret_cast<void*>(p.image),reinterpret_cast<void*>(p.bss),p.bss_bytes,1);
    if(p.error){discard(p);return;}p.loaded=true;
    uintptr_t symbol=0;p.step=4;p.error=sdk_call<uint32_t(*)(uintptr_t*,const void*,const char*)>(binding::ro_symbol)(&symbol,p.module,"shvPluginQuery");
    MemoryInfo mi{};uint32_t mempage=0;
    if(p.error||!symbol||sh_query_memory(&mi,&mempage,symbol)||!Memory::IsExecutable(symbol)){fail(p,p.error?p.error:0x10005);return;}
    p.base=mi.address;
    uint32_t mapped_magic=0,mapped_size=0;
    if(!code(p,symbol)||!Memory::Read(p.base+0x10,mapped_magic)||mapped_magic!=0x304f524e||!Memory::Read(p.base+0x18,mapped_size)||mapped_size!=p.image_bytes){fail(p,0x10005);return;}
    const auto* descriptor=reinterpret_cast<const ShvNxPlugin*(*)()>(symbol)();
    if(!NativeInvoker::CanYield()){
        NativeInvoker::AbandonCalls();fail(p,0x1001b);return;
    }
    if(!Memory::IsReadable(reinterpret_cast<uintptr_t>(descriptor),sizeof(*descriptor))||descriptor->size!=sizeof(*descriptor)||descriptor->abi!=SHV_NX_PLUGIN_ABI_VERSION||descriptor->caller_abi!=SHV_NX_CALLER_ABI_VERSION||descriptor->runtime_abi!=SHV_NX_RUNTIME_ABI_VERSION||!basename(descriptor->id,32,false)||!code(p,reinterpret_cast<uintptr_t>(descriptor->initialize))||!code(p,reinterpret_cast<uintptr_t>(descriptor->run))){fail(p,0x10006);return;}
    copy(p.id,descriptor->id);
    for(auto& other:plugins)if(&other!=&p&&other.loaded&&other.id[0]&&same_id(p.id,other.id)){fail(p,0x10007);return;}
    p.tls=sh_get_thread_pointer();
    p.api={sizeof(ShvNxApi),SHV_NX_PLUGIN_ABI_VERSION,SHV_NX_CALLER_ABI_VERSION,SHV_NX_RUNTIME_ABI_VERSION,&p,
        ::nativeInvokeContext,::getNativeCallStatus,::getLastNativeHash,::nativeIsAvailable,::scriptWait,register_native,write_data,read_data,
        ::nativeInit,::nativePush64,::nativeCall,::nativeGetCallContext,::nativeCallChecked,::getNativeHandler};
    p.step=5;
    // Stage custom handlers until initialization succeeds. A failed initializer
    // cannot publish a dangling callback into any script's native table.
    const bool initialized=descriptor->initialize(&p.api);
    if(!NativeInvoker::CanYield()){
        NativeInvoker::AbandonCalls();fail(p,0x1001b);return;
    }
    if(!initialized){fail(p,0x10008);return;}
    if(NativeTable::s_customNativeCount+p.pending_count>NativeTable::MaxCustomNatives){fail(p,0x1000e);return;}
    for(uint32_t i=0;i<p.pending_count;++i)if(NativeTable::GetHandler(p.pending[i].hash)){fail(p,0x1000e);return;}
    // Keep the image resident from the first published pointer onward, even
    // if a later registration unexpectedly fails.
    if(p.pending_count)p.published=true;
    for(uint32_t i=0;i<p.pending_count;++i)if(!NativeTable::RegisterCustom(p.pending[i].hash,p.pending[i].handler)){p.error=0x1000e;return;}
    p.run=descriptor->run;
    uintptr_t stop=0;
    // Missing optional export is normal for existing ABI 1 plugins.
    const auto stop_result=sdk_call<uint32_t(*)(uintptr_t*,const void*,const char*)>(binding::ro_symbol)(&stop,p.module,"shvPluginStop");
    if(!stop_result&&stop){if(!code(p,stop)){fail(p,0x10006);return;}p.cleanup=reinterpret_cast<ScriptFunction>(stop);}
    p.step=6;if(!ScriptManager::CommitReserved(reservation.token,p.run)){p.error=0x10009;return;}
    p.active=true;p.loaded_policy=p.policy;p.step=7;
}
bool better(const Candidate& a,const Candidate& b){
    if(a.policy!=b.policy)return a.policy>b.policy;
    return less(a.canonical,b.canonical);
}
void discover_plugin(const DirectoryEntry& entry,void*){
    Candidate next{};if(!parse_name(entry.name,next.canonical,next.policy))return;
    copy(next.filename,entry.name);++discovered;if(next.policy==Disabled)++disabled;
    // Never sort/copy the resident Plugin objects: api.plugin, scripts and
    // native registrations retain their address for the whole process.
    for(size_t i=0;i<count;++i){auto& p=plugins[i];if(!same_id(p.canonical,next.canonical))continue;
        if(p.present&&!same_id(p.filename,next.filename)){p.conflict=true;return;}
        p.present=true;p.policy=next.policy;copy(p.filename,next.filename);return;
    }
    for(size_t i=0;i<candidate_count;++i)if(same_id(candidates[i].canonical,next.canonical)){
        candidates[i].conflict=true;return;
    }
    size_t at=candidate_count;
    if(candidate_count<max_plugins)++candidate_count;
    else {++skipped;if(!better(next,candidates[candidate_count-1]))return;at=candidate_count-1;}
    while(at&&better(next,candidates[at-1])){candidates[at]=candidates[at-1];--at;}
    candidates[at]=next;
}
void discover_binding(const DirectoryEntry& entry,void* context){
    const char* name=entry.name;if(!basename(name,64,true)||!suffix(name,".snb"))return;
    auto& disk=*static_cast<Storage*>(context);char path[128]{};filename(path,"bindings/",name);size_t bytes=0;
    if(!disk.Read(path,manifest_buffer,sizeof(manifest_buffer),bytes)||!NativeManifest::Import(manifest_buffer,bytes))error=0x1000a;
}
bool any_queued(){for(const auto& p:plugins)if(p.queued||p.kill_queued)return true;return false;}
void scan(){
    error=0;discovered=disabled=skipped=0;plugin_entries=binding_entries=0;candidate_count=0;
    for(size_t i=0;i<count;++i){plugins[i].present=false;plugins[i].conflict=false;}
    {
        Storage disk;if(!disk.mounted){error=disk.error;return;}
        size_t bytes=0;if(disk.Read("native-bindings.bin",manifest_buffer,sizeof(manifest_buffer),bytes)){
            if(!NativeManifest::Import(manifest_buffer,bytes))error=0x1000a;
        }
        if(!disk.Scan("bindings",directory,16,discover_binding,&disk,binding_entries)){error=disk.error;return;}
        if(!disk.Scan("plugins",directory,16,discover_plugin,nullptr,plugin_entries)){error=disk.error;return;}
    }
    for(size_t i=0;i<candidate_count;++i){
        size_t slot=count;
        if(count==max_plugins){
            // Reclaim only unpublished catalog rows. Resident tokens never
            // move. Missing files and low-priority disabled/manual entries
            // must not permanently crowd new Auto plugins out of the list.
            for(size_t j=0;j<count;++j){const auto& p=plugins[j];
                if(p.loaded||p.registered||p.published||p.image||p.queued)continue;
                if(!p.present){slot=j;break;}
                if(p.policy<candidates[i].policy&&(slot==count||p.policy<plugins[slot].policy))slot=j;
            }
            if(slot==count){++skipped;continue;}
            if(plugins[slot].present)++skipped;
        } else ++count;
        auto& p=plugins[slot];p={};copy(p.filename,candidates[i].filename);copy(p.canonical,candidates[i].canonical);
        p.policy=candidates[i].policy;p.present=true;p.conflict=candidates[i].conflict;
        p.queued=p.policy==Auto&&!p.conflict;
    }
    for(size_t i=0;i<count;++i){auto& p=plugins[i];
        if(p.conflict){p.error=0x10017;p.queued=false;}
        else if(!p.present){p.error=0x10018;p.queued=false;}
        else if(p.error==0x10017||p.error==0x10018)p.error=0;
    }
}
}
uint32_t PluginLoader::Loaded(){return uint32_t(resident_count());}
bool PluginLoader::Pending(){return stage!=3||!reported||rescan_requested||any_queued()||running_mask()!=reported_running_mask;}
void PluginLoader::Tick(){
    if(!Memory::ResolveSdk())return;
    const auto tls=sh_get_thread_pointer();if(owner_tls&&owner_tls!=tls)return;owner_tls=tls;
    uint32_t frame=0;if(!Memory::Read(Memory::s_mainBase+binding::game_frame_counter,frame))return;
    if(frame_seen&&frame==last_frame)return;
    frame_seen=true;last_frame=frame;
    RuntimeConfig::LoadOnce();
    // This callback runs before scheduler dispatch, after the previous native
    // invocation has returned. Cancel only this plugin's cooperative entry;
    // its module, published handlers and other plugins remain untouched.
    for(auto& p:plugins)if(p.kill_queued||(p.active&&!p.cleanup_done&&!ScriptManager::Contains(p.run))){
        const bool killed=p.kill_queued;
        p.kill_queued=false;p.stopping=true;ScriptManager::Unregister(p.run);p.stopped=killed;p.cleanup_done=true;
        if(p.cleanup){++p.cleanup_calls;if(!ScriptManager::RunCleanup(p.cleanup))++p.cleanup_failures;}
        p.stopping=false;reported=false;
    }
    if(!stage||rescan_requested){
        rescan_requested=false;scan();reported=false;stage=any_queued()?1:3;
        if(stage==3)Report();
        return;
    }
    if(stage==3&&any_queued())stage=ro_ready?2:1;
    if(stage==1){
        if(!ro_ready){uintptr_t client=0;
            if(!Memory::Read(Memory::s_sdkBase+binding::ro_client,client)){error=0x1000b;stage=3;}
            else {
                if(!client)sdk_call<void(*)()>(binding::ro_initialize)();
                if(!Memory::Read(Memory::s_sdkBase+binding::ro_client,client)||!Memory::IsReadable(client,0x20)){error=0x1000b;stage=3;}
                else ro_ready=true;
            }
            if(!ro_ready){for(auto& p:plugins)if(p.queued){p.queued=false;p.error=error;}Report();return;}
        }
        stage=2;return;
    }
    if(stage==2){
        for(size_t i=0;i<count;++i){auto& p=plugins[i];if(!p.queued)continue;
            p.queued=false;
            if(!p.present)p.error=0x10018;
            else if(p.conflict)p.error=0x10017;
            else if(p.policy==Disabled)p.error=0x10014;
            else load(p);
            (void)NativeTable::EnsureEngineRegistration();reported=false;break;
        }
        if(!any_queued()){stage=3;Report();}
        return;
    }
    if(stage==3&&running_mask()!=reported_running_mask)reported=false;
    if(stage==3&&!reported)Report();
}
void PluginLoader::Report(){
    if(reported)return;
    char report[8192]{};size_t n=0;bool clipped=false;
    auto append=[&](const char* s){while(*s){if(n+32>=sizeof(report)){clipped=true;return;}report[n++]=*s++;}};
    auto field=[&](const char* key,uint64_t v){append(key);append("=0x");char tmp[16];unsigned at=0;do{tmp[at++]="0123456789abcdef"[v&15];v>>=4;}while(v);while(at){char digit[2]={tmp[--at],0};append(digit);}append("\n");};
    append("ScriptHookV-NX lifecycle test 1\n");field("loader_stage",stage);
    append("loader_stage_name=");append(stage==3?"complete":stage==2?"loading_plugins":stage==1?"initializing_loader":"discovering_plugins");append("\n");
    field("loader_error",error);append("loader_error_name=");append(error_name(error,0));append("\n");
    field("plugins_found",count);field("plugins_discovered",discovered);field("plugins_loaded",resident_count());field("plugins_skipped",skipped);field("plugins_disabled",disabled);
    field("plugin_directory_entries",plugin_entries);field("binding_directory_entries",binding_entries);field("directory_entry_limit",4096);field("native_manifests",NativeManifest::Count());
    field("resident_limit",RuntimeConfig::Residents());field("plugin_memory_budget",RuntimeConfig::MemoryBytes());field("plugin_memory_accounted",memory_used());field("last_action_error",last_action_error);
    uint32_t cleanup_count=0,cleanup_errors=0;for(const auto& p:plugins){cleanup_count+=p.cleanup_calls;cleanup_errors+=p.cleanup_failures;}
    field("cleanup_callbacks",cleanup_count);field("cleanup_failures",cleanup_errors);
    field("script_slot_limit",ScriptManager::SlotLimit());field("script_stack_bytes",ScriptManager::StackBytes);
    field("runtime_config_state",RuntimeConfig::State());field("runtime_config_error",RuntimeConfig::Error());
    field("runtime_config_applied_mask",RuntimeConfig::AppliedMask());field("runtime_config_invalid_mask",RuntimeConfig::InvalidMask());
    field("runtime_config_stack_error",RuntimeConfig::StackError());field("script_stack_allocated_bytes",ScriptManager::StackAllocationBytes());
    field("default_resident_limit",SHVNX_DEFAULT_RESIDENT_PLUGINS);field("default_script_slots",SHVNX_DEFAULT_SCRIPT_SLOTS);field("default_plugin_memory_mib",SHVNX_DEFAULT_PLUGIN_MEMORY_MIB);
    field("resident_limit_ceiling",SHVNX_MAX_RESIDENT_PLUGINS);field("script_slots_ceiling",SHVNX_MAX_SCRIPT_SLOTS);field("plugin_memory_mib_ceiling",SHVNX_MAX_PLUGIN_MEMORY_MIB);
    field("policy_request_slot",policy_slot);field("policy_requested",policy_requested);field("policy_previous",policy_previous);field("policy_effective",policy_effective);
    field("policy_rename_attempted",policy_rename_attempted);field("policy_rename_result",policy_rename_result);field("policy_readback_flags",policy_postcheck);
    uint32_t running_count=0;for(size_t i=0;i<count;++i)if(IsRunning(uint32_t(i)))++running_count;
    field("plugins_running",running_count);
    for(size_t i=0;i<count;++i){const auto& p=plugins[i];append("plugin=");append(p.filename);append("\n");field("slot",i);field("policy",p.policy);field("resident",p.loaded);field("running",IsRunning(uint32_t(i)));field("kill_queued",p.kill_queued);field("stopped",p.stopped);field("queued",p.queued);field("present",p.present);
        field("step",p.step);append("step_name=");append(step_name(p.step));append("\n");field("error",p.error);append("error_name=");append(error_name(p.error,p.step));append("\n");}
    // Leave room for a complete clipping field even when the body reaches its cap.
    const char* tail=clipped?"report_clipped=1\n":"report_clipped=0\n";while(*tail&&n+1<sizeof(report))report[n++]=*tail++;
    // A persistent SD failure must not mount/write every game frame. A new
    // scan or manager action explicitly requests another bounded attempt.
    reported_running_mask=running_mask();reported=true;Storage disk;(void)disk.Write("loader-report.txt",report,n);
}
uint32_t PluginLoader::Count(){return uint32_t(count);}
const char* PluginLoader::GetFilename(uint32_t idx){return idx<count?plugins[idx].filename:"";}
const char* PluginLoader::GetId(uint32_t idx){if(idx>=count)return "";return plugins[idx].id[0]?plugins[idx].id:plugins[idx].canonical;}
bool PluginLoader::GetInfo(uint32_t idx,uint32_t& status,uint32_t& memKb,uint32_t& step,uint32_t& err){
    if(idx>=count)return false;
    const auto& p=plugins[idx];
    if(p.queued||p.kill_queued)status=3;
    else if(p.error)status=2;
    else if(p.loaded&&p.active&&!IsRunning(idx))status=p.stopped?6:7;
    else if(p.loaded&&p.active)status=p.policy==p.loaded_policy?1:5;
    else if(p.loaded)status=5;
    else if(p.policy==Disabled)status=0;
    else status=4;
    memKb=uint32_t(memory_cost(p)/1024);step=p.step;err=p.error;return true;
}
const char* PluginLoader::GetStatusText(uint32_t idx){
    if(idx>=count)return "UNKNOWN";
    const auto& p=plugins[idx];
    if(p.kill_queued)return "STOP QUEUED - CODE STAYS RESIDENT";
    if(p.queued)return p.loaded?"RESTART QUEUED":"LOAD QUEUED";
    if(p.error)return error_name(p.error,p.step);
    if(p.loaded&&p.active&&!IsRunning(idx))return p.stopped?"STOPPED - LOAD NOW TO RESTART":"FINISHED - LOAD NOW TO RESTART";
    if(p.loaded)return p.policy!=p.loaded_policy?"LOADED - POLICY APPLIES NEXT LAUNCH":"LOADED - RESIDENT UNTIL EXIT";
    return p.policy==Disabled?"DISABLED":p.policy==Manual?"MANUAL - SELECT LOAD NOW":"AUTO - SELECT LOAD NOW OR RESTART";
}
uint32_t PluginLoader::GetPolicy(uint32_t idx){return idx<count?plugins[idx].policy:UINT32_MAX;}
uint32_t PluginLoader::LastActionError(){return last_action_error;}
bool PluginLoader::SetPolicy(uint32_t idx,uint32_t policy){
    if(!own_thread())return reject(0x10013);
    policy_slot=idx;policy_requested=policy;policy_previous=policy_effective=UINT32_MAX;
    policy_rename_attempted=false;policy_rename_result=UINT32_MAX;policy_postcheck=0;reported=false;
    if(idx>=count)return reject(0x10012);
    policy_previous=policy_effective=plugins[idx].policy;
    if(policy>Auto)return reject(0x10019);
    auto& p=plugins[idx];
    if(!p.present)return reject(0x10018);
    if(p.conflict)return reject(0x10017);
    if(p.policy==policy){
        Storage disk;size_t bytes=0;char path[128]{};filename(path,"plugins/",p.filename);
        if(!disk.mounted||!disk.Size(path,bytes))return reject(disk.error?disk.error:0x1001d);
        policy_postcheck=1;last_action_error=0;return true;
    }
    char next[64]{};copy(next,p.canonical);size_t at=length(next);
    const char* ending=policy==Disabled?".disabled":policy==Manual?".manual":"";
    while(*ending)next[at++]=*ending++;
    next[at]=0;
    char old_path[128]{},new_path[128]{};filename(old_path,"plugins/",p.filename);filename(new_path,"plugins/",next);
    {
        Storage disk;if(!disk.mounted)return reject(disk.error?disk.error:0x10013);
        const bool renamed=disk.Rename(old_path,new_path);
        policy_rename_attempted=disk.rename_attempted;policy_rename_result=disk.rename_result;policy_postcheck=disk.rename_postcheck;
        if(!renamed)return reject(disk.error?disk.error:0x1001d);
    }
    if(p.policy==Disabled&&disabled)--disabled;
    if(policy==Disabled)++disabled;
    copy(p.filename,next);p.policy=policy;if(policy==Disabled)p.queued=false;
    policy_effective=policy;
    reported=false;last_action_error=0;return true;
}
bool PluginLoader::Toggle(uint32_t idx){
    if(!own_thread())return reject(0x10013);
    if(idx>=count)return reject(0x10012);
    return SetPolicy(idx,(plugins[idx].policy+1)%3);
}
bool PluginLoader::RequestLoad(uint32_t idx){
    if(!own_thread())return reject(0x10013);
    if(idx>=count)return reject(0x10012);
    auto& p=plugins[idx];
    if(p.stopping)return reject(0x10013);
    if(p.policy==Disabled)return reject(0x10014);
    if(p.loaded){
        if(!p.active||!p.run)return reject(p.error?p.error:0x1001a);
        if(!p.kill_queued&&IsRunning(idx)){last_action_error=0;return true;}
        p.queued=true;p.error=0;reported=false;last_action_error=0;return true;
    }
    if(!p.present)return reject(0x10018);
    if(p.conflict)return reject(0x10017);
    if(resident_count()>=RuntimeConfig::Residents())return reject(0x10015);
    p.queued=true;p.error=0;p.step=0;reported=false;last_action_error=0;return true;
}
bool PluginLoader::IsRunning(uint32_t idx){
    return own_thread()&&idx<count&&plugins[idx].loaded&&plugins[idx].active&&ScriptManager::Contains(plugins[idx].run);
}
bool PluginLoader::Kill(uint32_t idx){
    if(!own_thread())return reject(0x10013);
    if(idx>=count)return reject(0x10012);
    if(plugins[idx].stopping)return reject(0x10013);
    if(!IsRunning(idx))return reject(0x1001c);
    plugins[idx].kill_queued=true;reported=false;last_action_error=0;return true;
}
bool PluginLoader::RescanAndReload(){
    if(!own_thread())return reject(0x10013);
    rescan_requested=true;reported=false;last_action_error=0;return true;
}
}
