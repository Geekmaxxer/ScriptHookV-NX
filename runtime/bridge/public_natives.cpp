#include "public_natives.hpp"
#include "../core/storage.hpp"
#include "../script/native_table.hpp"
#include "../plugins/plugin_loader.hpp"
namespace ScriptHook {
namespace {
struct Context { uint64_t* result; uint32_t count,pad; uint64_t* args; };
struct Record { uint32_t magic,version,generation,bytes,checksum; uint32_t data[2048]; };
uint32_t crc(const void* data,size_t n,uint32_t c=~0u) {
    auto* p=static_cast<const uint8_t*>(data);
    while(n--) {c^=*p++;for(unsigned i=0;i<8;++i)c=(c>>1)^(0xedb88320u&uint32_t(0-int32_t(c&1)));}
    return c;
}
uint32_t checksum(const Record& r) {return ~crc(r.data,r.bytes,crc(&r,16));}
bool valid(const Record& r,size_t got) {return r.magic==0x584e4853&&r.version==1&&r.generation&&r.bytes&&r.bytes<=8192&&!(r.bytes%4)&&got==20+r.bytes&&r.checksum==checksum(r);}
void names(char (&a)[40],char (&b)[40],uint32_t id) {
    constexpr char prefix[]="settings/nsc-";size_t n=0;for(char c:prefix){if(!c)break;a[n]=b[n]=c;++n;}
    for(int i=7;i>=0;--i)a[n]=b[n]="0123456789abcdef"[(id>>(i*4))&15],++n;
    const char* tail="/a.bin";for(unsigned i=0;tail[i];++i){a[n]=b[n]=tail[i];++n;}
    a[n]=b[n]=0;b[n-5]='b';
}
bool prepare_names(Storage& disk,char (&a)[40],char (&b)[40]) {
    if(!disk.mounted)return false;
    char directory[40]{};size_t n=0;while(a[n]){directory[n]=a[n];++n;}
    if(n<6)return false;
    directory[n-6]=0;
    if(!disk.Directory(directory))return false;
    char old_a[40]{},old_b[40]{};
    for(size_t i=0;i<=n;++i){old_a[i]=a[i];old_b[i]=b[i];}
    old_a[n-6]=old_b[n-6]='-';
    return disk.MigrateFile(old_a,a)&&disk.MigrateFile(old_b,b);
}
bool args(void* raw,uint64_t*& words,uint32_t& count,uint32_t& id) {
    auto p=reinterpret_cast<uintptr_t>(raw);if(!Memory::IsReadable(p,sizeof(Context)))return false;
    auto& c=*static_cast<Context*>(raw);
    if(!Memory::IsWritable(reinterpret_cast<uintptr_t>(c.result),8))return false;
    if(c.count<2||c.count>3||!Memory::IsReadable(reinterpret_cast<uintptr_t>(c.args),c.count*8)){*c.result=0;return false;}
    words=reinterpret_cast<uint64_t*>(c.args[0]);count=uint32_t(c.args[1]);id=c.count==3?uint32_t(c.args[2]):0;
    *c.result=0;
    if(!id){uintptr_t thread=0;uint8_t active=0;
        const auto tls=sh_get_thread_pointer();
        if(!Memory::Read(tls+binding::script_tls_active,active)||active!=1||!Memory::Read(tls+binding::script_tls_thread,thread)||!Memory::Read(thread+0xc,id))return false;
    }
    return id&&count&&count<=2048;
}
void save(void* raw) {
    uint64_t* words=nullptr;uint32_t count=0,id=0;if(!args(raw,words,count,id)||!Memory::IsReadable(reinterpret_cast<uintptr_t>(words),size_t(count)*8))return;
    char an[40]{},bn[40]{};names(an,bn,id);Storage disk;if(!prepare_names(disk,an,bn))return;
    Record a{},b{};size_t got=0;bool va=disk.Read(an,&a,sizeof(a),got)&&valid(a,got),vb=disk.Read(bn,&b,sizeof(b),got)&&valid(b,got);
    const Record* latest=va&&vb?(int32_t(b.generation-a.generation)>0?&b:&a):va?&a:vb?&b:nullptr;
    Record out{0x584e4853,1,latest?latest->generation+1:1,count*4,0,{}};
    if(!out.generation)out.generation=1;
    for(uint32_t i=0;i<count;++i)out.data[i]=uint32_t(words[i]);
    out.checksum=checksum(out);
    *static_cast<Context*>(raw)->result=disk.Write(latest==&a?bn:an,&out,20+out.bytes)?1:0;
}
void load(void* raw) {
    uint64_t* words=nullptr;uint32_t count=0,id=0;if(!args(raw,words,count,id)||!Memory::IsWritable(reinterpret_cast<uintptr_t>(words),size_t(count)*8))return;
    char an[40]{},bn[40]{};names(an,bn,id);Storage disk;if(!prepare_names(disk,an,bn))return;
    Record a{},b{};size_t got=0;bool va=disk.Read(an,&a,sizeof(a),got)&&valid(a,got),vb=disk.Read(bn,&b,sizeof(b),got)&&valid(b,got);
    const Record* chosen=va&&vb?(int32_t(b.generation-a.generation)>0?&b:&a):va?&a:vb?&b:nullptr;
    if(!chosen||chosen->bytes>count*4)return;
    for(uint32_t i=0;i<chosen->bytes/4;++i)words[i]=uint64_t(int64_t(int32_t(chosen->data[i])));
    *static_cast<Context*>(raw)->result=chosen->bytes/4;
}
void available(void* raw) {
    if(!Memory::IsReadable(reinterpret_cast<uintptr_t>(raw),sizeof(Context)))return;
    auto& c=*static_cast<Context*>(raw);if(!Memory::IsWritable(reinterpret_cast<uintptr_t>(c.result),8))return;
    if(c.count!=2||!Memory::IsReadable(reinterpret_cast<uintptr_t>(c.args),16)){*c.result=0;return;}
    const uint64_t hash=uint32_t(c.args[0])|(uint64_t(uint32_t(c.args[1]))<<32);
    const auto h=NativeTable::GetHandler(hash);*c.result=h&&reinterpret_cast<uintptr_t>(h)!=Memory::s_mainBase+binding::missing_native;
}
bool overlap(uintptr_t a,size_t an,uintptr_t b,size_t bn) {
    return an&&bn&&a<b+bn&&b<a+an;
}
struct ManagerCall {
    Context context{}; uint64_t values[5]{}; uintptr_t address=0;
    bool Read(void* raw,uint32_t expected) {
        address=reinterpret_cast<uintptr_t>(raw);
        if(!Memory::Read(address,context)||
           !Memory::IsWritable(reinterpret_cast<uintptr_t>(context.result),8)||
           overlap(reinterpret_cast<uintptr_t>(context.result),8,address,sizeof(Context)))return false;
        // The VM may reuse argument storage for its return value. Snapshot
        // all arguments before writing any result or output parameter.
        if(context.count!=expected||expected>5||
           (expected&&!Memory::Read(reinterpret_cast<uintptr_t>(context.args),values[0]))) {
            *context.result=0;return false;
        }
        if(expected) {
            if(!Memory::IsReadable(reinterpret_cast<uintptr_t>(context.args),expected*8)){*context.result=0;return false;}
            for(uint32_t i=0;i<expected;++i)values[i]=context.args[i];
        }
        *context.result=0;return true;
    }
    uint32_t Index() const {return uint32_t(values[0]);}
    bool Outputs(unsigned first=1,unsigned last=3) const {
        for(unsigned i=first;i<=last;++i) {
            const auto p=uintptr_t(values[i]);
            if((p&7)||!Memory::IsWritable(p,8)||overlap(p,8,address,sizeof(Context))||
               overlap(p,8,reinterpret_cast<uintptr_t>(context.result),8)||
               overlap(p,8,reinterpret_cast<uintptr_t>(context.args),context.count*8))return false;
            for(unsigned j=first;j<i;++j)if(overlap(p,8,uintptr_t(values[j]),8))return false;
        }
        return true;
    }
};
void plugin_count(void* raw) {ManagerCall c;if(c.Read(raw,0))*c.context.result=PluginLoader::Count();}
void plugin_name(void* raw) {ManagerCall c;if(c.Read(raw,1))*c.context.result=reinterpret_cast<uintptr_t>(PluginLoader::GetFilename(c.Index()));}
void plugin_id(void* raw) {ManagerCall c;if(c.Read(raw,1))*c.context.result=reinterpret_cast<uintptr_t>(PluginLoader::GetId(c.Index()));}
void plugin_status(void* raw) {
    ManagerCall c;if(!c.Read(raw,4)||!c.Outputs())return;
    uint32_t status=0,mem=0,step=0,error=0;
    if(!PluginLoader::GetInfo(c.Index(),status,mem,step,error))return;
    *reinterpret_cast<uint64_t*>(c.values[1])=status;
    *reinterpret_cast<uint64_t*>(c.values[2])=mem;
    *reinterpret_cast<uint64_t*>(c.values[3])=step;
    *c.context.result=1;
}
void plugin_status_text(void* raw) {ManagerCall c;if(c.Read(raw,1))*c.context.result=reinterpret_cast<uintptr_t>(PluginLoader::GetStatusText(c.Index()));}
void plugin_toggle(void* raw) {ManagerCall c;if(c.Read(raw,1))*c.context.result=PluginLoader::Toggle(c.Index());}
void plugin_reload(void* raw) {ManagerCall c;if(c.Read(raw,0))*c.context.result=PluginLoader::RescanAndReload();}
void plugin_load(void* raw) {ManagerCall c;if(c.Read(raw,1))*c.context.result=PluginLoader::RequestLoad(c.Index());}
void plugin_get_policy(void* raw) {ManagerCall c;if(c.Read(raw,1))*c.context.result=PluginLoader::GetPolicy(c.Index());}
void plugin_set_policy(void* raw) {ManagerCall c;if(c.Read(raw,2))*c.context.result=PluginLoader::SetPolicy(c.Index(),uint32_t(c.values[1]));}
void plugin_action_error(void* raw) {ManagerCall c;if(c.Read(raw,0))*c.context.result=PluginLoader::LastActionError();}
void plugin_kill(void* raw) {ManagerCall c;if(c.Read(raw,1))*c.context.result=PluginLoader::Kill(c.Index());}
void plugin_running(void* raw) {ManagerCall c;if(c.Read(raw,1))*c.context.result=PluginLoader::IsRunning(c.Index());}
void plugin_timing(void* raw) {
    ManagerCall c;if(!c.Read(raw,5)||!c.Outputs(2,4))return;
    uint64_t calls=0,total=0,worst=0;
    if(!PluginLoader::GetTiming(c.Index(),uint32_t(c.values[1]),calls,total,worst))return;
    *reinterpret_cast<uint64_t*>(c.values[2])=calls;
    *reinterpret_cast<uint64_t*>(c.values[3])=total;
    *reinterpret_cast<uint64_t*>(c.values[4])=worst;
    *c.context.result=1;
}
void plugin_version(void* raw) {ManagerCall c;if(c.Read(raw,0))*c.context.result=3;}
}
bool RegisterPublicNatives() {
    return NativeTable::RegisterCustom(0x5348564e58535631ULL,save)&&
           NativeTable::RegisterCustom(0x5348564e584c4431ULL,load)&&
           NativeTable::RegisterCustom(0x5348564e58415631ULL,available)&&
           NativeTable::RegisterCustom(0x5348564e58504331ULL,plugin_count)&&
           NativeTable::RegisterCustom(0x5348564e58504e31ULL,plugin_name)&&
           NativeTable::RegisterCustom(0x5348564e58504931ULL,plugin_id)&&
           NativeTable::RegisterCustom(0x5348564e58505331ULL,plugin_status)&&
           NativeTable::RegisterCustom(0x5348564e58505431ULL,plugin_status_text)&&
           NativeTable::RegisterCustom(0x5348564e58505447ULL,plugin_toggle)&&
           NativeTable::RegisterCustom(0x5348564e5850524cULL,plugin_reload)&&
           NativeTable::RegisterCustom(0x5348564e58504c31ULL,plugin_load)&&
           NativeTable::RegisterCustom(0x5348564e58505031ULL,plugin_get_policy)&&
           NativeTable::RegisterCustom(0x5348564e58505032ULL,plugin_set_policy)&&
           NativeTable::RegisterCustom(0x5348564e58504531ULL,plugin_action_error)&&
           NativeTable::RegisterCustom(0x5348564e58504b31ULL,plugin_kill)&&
           NativeTable::RegisterCustom(0x5348564e58505231ULL,plugin_running)&&
           NativeTable::RegisterCustom(0x5348564e58505631ULL,plugin_version)&&
           NativeTable::RegisterCustom(0x5348564e58504d31ULL,plugin_timing);
}
}
