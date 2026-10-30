#include "plugin_heap.hpp"
#include "plugin_loader.hpp"
#include "../core/storage.hpp"
namespace ScriptHook {
namespace {
constexpr uint32_t ArenaCount=128,BlockCount=2048,OwnerCount=16,End=UINT32_MAX;
constexpr size_t PoolBytes=65536,MaxAlignment=4096;
struct Owner {void* token;uint64_t requested,charged,live,pools,peak_requested,peak_charged,failures,release_failures;};
struct Arena {void* owner;uintptr_t base;size_t bytes;uint32_t head,tail;bool pages_released,release_failed;};
struct Block {void* owner;uintptr_t address;size_t bytes,alignment;uint32_t arena,next,previous,hash_slot;};
constexpr uint32_t LookupCount=4096;
Owner owners[OwnerCount]{};Arena arenas[ArenaCount]{};Block blocks[BlockCount]{};uint32_t lookup[LookupCount]{};
bool busy{};uint32_t highwater{},free_head=End,tombstones{};
struct Lock {bool owned;Lock():owned(!busy){if(owned)busy=true;}~Lock(){if(owned)busy=false;}};
Owner* owner(void* token){
    if(!token)return nullptr;
    for(auto& o:owners)if(o.token==token)return &o;
    for(auto& o:owners)if(!o.token){o.token=token;return &o;}
    return nullptr;
}
void fail(Owner* o){if(o&&o->failures!=UINT64_MAX)++o->failures;}
void release_fail(Owner* o){if(o&&o->release_failures!=UINT64_MAX)++o->release_failures;}
void peak(Owner& o){if(o.requested>o.peak_requested)o.peak_requested=o.requested;if(o.charged>o.peak_charged)o.peak_charged=o.charged;}
uint32_t hash(void* token,uintptr_t address){return uint32_t(((address>>4)^(address>>19)^(reinterpret_cast<uintptr_t>(token)>>5))*0x9e3779b1U)&(LookupCount-1);}
uint32_t find(void* token,const void* p){
    const auto address=reinterpret_cast<uintptr_t>(p);const auto start=hash(token,address);
    for(uint32_t n=0;n<LookupCount;++n){const auto value=lookup[(start+n)&(LookupCount-1)];if(!value)return End;if(value!=End&&blocks[value-1].owner==token&&blocks[value-1].address==address)return value-1;}
    return End;
}
void index(uint32_t i){
    auto& b=blocks[i];const auto start=hash(b.owner,b.address);
    for(uint32_t n=0;n<LookupCount;++n){const auto at=(start+n)&(LookupCount-1);if(!lookup[at]||lookup[at]==End){if(lookup[at]==End)--tombstones;lookup[at]=i+1;b.hash_slot=at;return;}}
}
void compact_index(){if(tombstones>BlockCount/2){for(auto& v:lookup)v=0;tombstones=0;for(uint32_t i=0;i<highwater;++i)if(blocks[i].owner)index(i);}}
bool gap(const Arena& a,size_t bytes,size_t alignment,uintptr_t& result,uint32_t& previous,uint32_t& next){
    if(a.tail!=End){
        const auto& tail=blocks[a.tail];const uintptr_t at=(tail.address+tail.bytes+alignment-1)&~uintptr_t(alignment-1);
        if(at<=a.base+a.bytes&&bytes<=a.base+a.bytes-at){result=at;previous=a.tail;next=End;return true;}
    }
    uintptr_t at=a.base;previous=End;next=a.head;
    for(uint32_t steps=0;steps<=BlockCount;++steps){
        const uintptr_t limit=next==End?a.base+a.bytes:blocks[next].address;
        const uintptr_t aligned=(at+alignment-1)&~uintptr_t(alignment-1);
        if(aligned<=limit&&bytes<=limit-aligned){result=aligned;return true;}
        if(next==End)return false;
        at=blocks[next].address+blocks[next].bytes;previous=next;next=blocks[next].next;
    }
    return false;
}
void* allocate(Owner& o,size_t bytes,size_t alignment,uint32_t flags){
    if(!bytes||!alignment||alignment>MaxAlignment||(alignment&(alignment-1))||(flags&~SHV_NX_MEMORY_ZERO)||bytes>SIZE_MAX-4095){fail(&o);return nullptr;}
    compact_index();
    if(alignment<16)alignment=16;
    const uint32_t slot=free_head!=End?free_head:(highwater<BlockCount?highwater:End);
    if(slot==End){fail(&o);return nullptr;}
    uint32_t ai=End,previous=End,next=End;uintptr_t address=0;
    for(uint32_t i=0;i<ArenaCount;++i)if(arenas[i].owner==o.token&&!arenas[i].pages_released&&!arenas[i].release_failed&&gap(arenas[i],bytes,alignment,address,previous,next)){ai=i;break;}
    if(ai==End){
        for(uint32_t i=0;i<ArenaCount;++i)if(!arenas[i].owner){ai=i;break;}
        size_t capacity=(bytes+4095)&~size_t(4095);if(capacity<PoolBytes)capacity=PoolBytes;
        if(ai==End||!PluginLoader::ReserveExtraMemory(capacity)){fail(&o);return nullptr;}
        uintptr_t at=0;const auto result=sdk_call<uint32_t(*)(uintptr_t*,size_t)>(binding::allocate_pages)(&at,capacity);
        if(result||!at){PluginLoader::ReleaseExtraMemory(capacity);fail(&o);return nullptr;}
        // The reviewed allocator guarantees a page-aligned mapped region on
        // success. Retain an unexpected nonzero allocation rather than forget it.
        arenas[ai]={o.token,at,capacity,End,End,false,false};o.charged+=capacity;++o.pools;
        if((at&4095)||at>UINTPTR_MAX-capacity||!Memory::IsWritable(at,capacity)){
            arenas[ai].release_failed=true;release_fail(&o);fail(&o);peak(o);return nullptr;
        }
        address=at;previous=next=End;
    }
    if(slot==free_head)free_head=blocks[slot].next;else ++highwater;
    blocks[slot]={o.token,address,bytes,alignment,ai,next,previous,0};o.requested+=bytes;++o.live;
    if(previous==End)arenas[ai].head=slot;else blocks[previous].next=slot;
    if(next!=End)blocks[next].previous=slot;
    if(next==End)arenas[ai].tail=slot;
    index(slot);
    if(flags&SHV_NX_MEMORY_ZERO){auto* p=reinterpret_cast<uint8_t*>(address);for(size_t i=0;i<bytes;++i)p[i]=0;}
    peak(o);return reinterpret_cast<void*>(address);
}
void free_block(uint32_t index){
    auto& b=blocks[index];auto& a=arenas[b.arena];
    if(b.previous==End)a.head=b.next;else blocks[b.previous].next=b.next;
    if(b.next==End)a.tail=b.previous;else blocks[b.next].previous=b.previous;
    lookup[b.hash_slot]=End;++tombstones;
    if(auto* o=owner(b.owner)){o->requested-=b.bytes;--o->live;}
    b={};b.next=free_head;free_head=index;
}
bool trim(Owner& o){
    bool good=true;
    for(auto& a:arenas)if(a.owner==o.token&&a.head==End){
        // The target has two result-bearing release phases. After successful
        // page release, retry only FreeAddressRegion if that phase fails.
        if(!a.pages_released){
            if(sdk_call<uint32_t(*)(uintptr_t,size_t)>(binding::free_pages)(a.base,a.bytes)){a.release_failed=true;release_fail(&o);good=false;continue;}
            a.pages_released=true;
        }
        if(sdk_call<uint32_t(*)(uintptr_t)>(binding::free_address)(a.base)){a.release_failed=true;release_fail(&o);good=false;continue;}
        PluginLoader::ReleaseExtraMemory(a.bytes);o.charged-=a.bytes;--o.pools;a={};
    }
    return good;
}
}
void* PluginHeap::Allocate(void* token,size_t bytes,size_t alignment,uint32_t flags){Lock lock;auto* o=owner(token);if(!lock.owned||!o){fail(o);return nullptr;}return allocate(*o,bytes,alignment,flags);}
bool PluginHeap::Free(void* token,void* p){Lock lock;auto* o=owner(token);if(!lock.owned||!o){fail(o);return false;}if(!p)return true;const auto i=find(token,p);if(i==End){fail(o);return false;}free_block(i);return true;}
size_t PluginHeap::Size(void* token,const void* p){Lock lock;if(!lock.owned||!p)return 0;const auto i=find(token,p);return i==End?0:blocks[i].bytes;}
void* PluginHeap::Reallocate(void* token,void* p,size_t bytes){
    Lock lock;auto* o=owner(token);if(!lock.owned||!o){fail(o);return nullptr;}
    if(!p)return allocate(*o,bytes,16,0);
    const auto i=find(token,p);if(i==End){fail(o);return nullptr;}
    if(!bytes){free_block(i);return nullptr;}
    auto& b=blocks[i];
    if(bytes<=b.bytes){o->requested-=b.bytes-bytes;b.bytes=bytes;return p;}
    const auto& a=arenas[b.arena];const auto limit=b.next==End?a.base+a.bytes:blocks[b.next].address;
    if(bytes<=limit-b.address){o->requested+=bytes-b.bytes;b.bytes=bytes;peak(*o);return p;}
    void* next=allocate(*o,bytes,b.alignment,0);if(!next)return nullptr;
    auto* out=static_cast<uint8_t*>(next);const auto* in=static_cast<const uint8_t*>(p);for(size_t j=0;j<b.bytes;++j)out[j]=in[j];free_block(i);return next;
}
bool PluginHeap::Stats(void* token,ShvNxMemoryStats& out){
    Lock lock;auto* o=owner(token);if(!lock.owned||!o)return false;
    out={};out.size=sizeof(out);out.abi=SHV_NX_MEMORY_ABI_VERSION;
    out.requested_bytes=o->requested;out.charged_bytes=o->charged;out.allocations=o->live;out.arenas=o->pools;
    out.peak_requested_bytes=o->peak_requested;out.peak_charged_bytes=o->peak_charged;out.failures=o->failures;out.release_failures=o->release_failures;return true;
}
bool PluginHeap::Trim(void* token){Lock lock;auto* o=owner(token);return lock.owned&&o&&trim(*o);}
bool PluginHeap::Discard(void* token){Lock lock;auto* o=owner(token);if(!lock.owned||!o)return false;for(uint32_t i=0;i<BlockCount;++i)if(blocks[i].owner==token)free_block(i);return trim(*o);}
}
