#include "native_manifest.hpp"
#include "native_table.hpp"
#include "../core/memory.hpp"
#include "bindings.hpp"
namespace ScriptHook {
namespace {
struct Record {uint32_t name,code,natives,count;uint8_t entry[32];};
struct Entry {uint32_t index,reserved;uint64_t hash;};
alignas(8) uint8_t manifest[65536];size_t length;
uint32_t records;
uint32_t crc(const uint8_t* p,size_t n){uint32_t c=~0u;while(n--){c^=*p++;for(unsigned i=0;i<8;++i)c=(c>>1)^(0xedb88320u&uint32_t(0-int32_t(c&1)));}return ~c;}
}
bool NativeManifest::Import(const void* input,size_t size) {
    if(size<16||size>sizeof(manifest)||!Memory::IsReadable(reinterpret_cast<uintptr_t>(input),size)||reinterpret_cast<uintptr_t>(input)%8)return false;
    const auto* h=static_cast<const uint32_t*>(input);const auto* bytes=static_cast<const uint8_t*>(input);
    if(h[0]!=0x424e4853||h[1]!=1||h[2]>64||h[3]!=crc(bytes+16,size-16))return false;
    size_t at=16;
    for(uint32_t i=0;i<h[2];++i){
        if(at+sizeof(Record)>size)return false;
        const auto& r=*reinterpret_cast<const Record*>(bytes+at);at+=sizeof(r);
        if(!r.name||r.code<32||!r.natives||r.natives>65535||!r.count||r.count>256||size-at<size_t(r.count)*sizeof(Entry))return false;
        for(uint32_t j=0;j<r.count;++j){const auto& e=*reinterpret_cast<const Entry*>(bytes+at+j*sizeof(Entry));if(e.index>=r.natives||e.reserved||!e.hash)return false;}
        at+=r.count*sizeof(Entry);
    }
    if(at!=size)return false;
    // Merge independent per-mod manifests. Duplicate identities must be exact;
    // conflicting descriptions are rejected before mutating any saved record.
    size_t append=0;uint32_t added=0;at=16;
    for(uint32_t i=0;i<h[2];++i){
        const auto& r=*reinterpret_cast<const Record*>(bytes+at);
        const size_t extent=sizeof(r)+r.count*sizeof(Entry);bool duplicate=false;size_t previous=16;
        for(uint32_t j=0;j<records;++j){const auto& old=*reinterpret_cast<const Record*>(manifest+previous);const auto old_size=sizeof(old)+old.count*sizeof(Entry);
            if(old.name==r.name){if(old_size!=extent)return false;for(size_t k=0;k<extent;++k)if(manifest[previous+k]!=bytes[at+k])return false;duplicate=true;}
            previous+=old_size;
        }
        // Reject duplicate names within this input as well.
        size_t earlier=16;for(uint32_t j=0;j<i;++j){const auto& old=*reinterpret_cast<const Record*>(bytes+earlier);if(old.name==r.name)return false;earlier+=sizeof(old)+old.count*sizeof(Entry);}
        if(!duplicate){append+=extent;++added;}at+=extent;
    }
    if(records+added>64 || (length?length:16)+append>sizeof(manifest))return false;
    size_t end=length?length:16;at=16;
    for(uint32_t i=0;i<h[2];++i){const auto& r=*reinterpret_cast<const Record*>(bytes+at);const size_t extent=sizeof(r)+r.count*sizeof(Entry);bool duplicate=false;size_t previous=16;
        for(uint32_t j=0;j<records;++j){const auto& old=*reinterpret_cast<const Record*>(manifest+previous);if(old.name==r.name)duplicate=true;previous+=sizeof(old)+old.count*sizeof(Entry);}
        if(!duplicate){for(size_t k=0;k<extent;++k)manifest[end+k]=bytes[at+k];end+=extent;}
        at+=extent;
    }
    length=end;records+=added;
    auto* header=reinterpret_cast<uint32_t*>(manifest);header[0]=0x424e4853;header[1]=1;header[2]=records;header[3]=crc(manifest+16,length-16);return true;
}
uint32_t NativeManifest::Count(){return records;}
bool NativeManifest::Repair(void* thread) {
    if(!records)return true;
    uint32_t name=0;
    if(!Memory::Read(reinterpret_cast<uintptr_t>(thread)+0xc,name))return false;
    const Record* record=nullptr;const Entry* entries=nullptr;size_t at=16;
    for(uint32_t i=0;i<records;++i){const auto* r=reinterpret_cast<const Record*>(manifest+at);at+=sizeof(*r);if(r->name==name){record=r;entries=reinterpret_cast<const Entry*>(manifest+at);break;}at+=r->count*sizeof(Entry);}
    if(!record)return true;
    struct Lock{uintptr_t p=0;Lock(){uint8_t b=0;auto addr=Memory::s_mainBase+binding::program_mutex;if(Memory::Read(addr,b)&&b==1){reinterpret_cast<void(*)(void*)>(Memory::s_mainBase+binding::lock_mutex)(reinterpret_cast<void*>(addr));p=addr;}}~Lock(){if(p)reinterpret_cast<void(*)(void*)>(Memory::s_mainBase+binding::unlock_mutex)(reinterpret_cast<void*>(p));}}lock;
    const auto table=Memory::s_mainBase+binding::program_table;uint8_t node=0;uintptr_t program=0;
    if(!Memory::Read(table+1+(name&31),node))return false;
    for(unsigned i=0;node&&i<256;++i){uintptr_t p=0;uint32_t hash=0;if(!Memory::Read(table+0xd8+uintptr_t(node)*8,p)||!Memory::Read(p+0x58,hash))return false;if(hash==name){program=p;break;}if(!Memory::Read(table+0x21+node,node))return false;}
    uint32_t code=0,count=0;uintptr_t blocks=0,first=0,natives=0;
    auto reject=[](){++NativeTable::s_cacheRejected;return false;};
    if(!program||!Memory::Read(program+0x1c,code)||!Memory::Read(program+0x2c,count)||code!=record->code||count!=record->natives||
       !Memory::Read(program+0x10,blocks)||!Memory::Read(blocks,first)||!Memory::Read(program+0x40,natives)||!Memory::IsReadable(first,32))return reject();
    for(unsigned i=0;i<32;++i)if(reinterpret_cast<const uint8_t*>(first)[i]!=record->entry[i])return reject();
    // Validate all relevant slots before changing one. A manifest cannot replace
    // arbitrary game handlers; only registered custom hashes and the stock trap.
    for(uint32_t i=0;i<record->count;++i){
        const auto& e=entries[i];NativeHandler handler=nullptr;
        for(size_t j=0;j<NativeTable::s_customNativeCount;++j)if(NativeTable::s_customNatives[j].hash==e.hash)handler=NativeTable::s_customNatives[j].handler;
        uintptr_t value=0,target=natives+e.index*8;
        if(!handler) {
            if(!Memory::Read(target,value))return reject();
            if(value==Memory::s_mainBase+binding::missing_native)return false;
            continue;
        }
        if(!Memory::IsWritable(target,8)||!Memory::Read(target,value)||(value!=Memory::s_mainBase+binding::missing_native&&value!=reinterpret_cast<uintptr_t>(handler)))return reject();
    }
    for(uint32_t i=0;i<record->count;++i){const auto& e=entries[i];for(size_t j=0;j<NativeTable::s_customNativeCount;++j)if(NativeTable::s_customNatives[j].hash==e.hash){
        auto* target=reinterpret_cast<uintptr_t*>(natives+e.index*8);uintptr_t old=Memory::s_mainBase+binding::missing_native;
        if(__atomic_compare_exchange_n(target,&old,reinterpret_cast<uintptr_t>(NativeTable::s_customNatives[j].handler),false,__ATOMIC_RELEASE,__ATOMIC_RELAXED))++NativeTable::s_cacheRepairs;
    }}
    return true;
}
}
