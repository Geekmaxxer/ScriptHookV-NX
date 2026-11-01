#include "plugin_heap.hpp"
#include "plugin_loader.hpp"
#include "plugin_resources.hpp"
#include "../core/storage.hpp"
#include "../core/slot_store.hpp"
namespace ScriptHook {
namespace {
constexpr uint32_t End=UINT32_MAX;
constexpr size_t PoolBytes=65536,MaxAlignment=4096;
struct Owner {void* token;uint64_t requested,charged,live,pools,peak_requested,peak_charged,failures,release_failures;};
struct Arena {void* owner;uintptr_t base;size_t bytes,occupied;uint32_t head,tail;bool pages_released,release_failed;};
struct Block {void* owner;uintptr_t address;size_t bytes,alignment;uint32_t arena,next,previous,hash_slot;bool resource_pinned;};
SlotStore<Owner,16> owners{};
SlotStore<Arena,128,16> arenas{};
SlotStore<Block,2048,256> blocks{};
uint32_t initial_lookup[4096]{};
uint32_t* lookup=initial_lookup;
size_t lookup_capacity=4096,tombstones{},metadata_bytes{};
bool busy{};uint32_t highwater{},free_head=End,live_blocks{};
struct Lock {bool owned;Lock():owned(!busy){if(owned)busy=true;}~Lock(){if(owned)busy=false;}};

template<class Operation>bool admit_metadata(size_t bytes,Operation operation){
    if(!bytes||bytes>SIZE_MAX-4095)return false;
    const auto charge=(bytes+4095)&~size_t(4095);
    if(!PluginLoader::ReserveExtraMemory(charge))return false;
    const auto before=RuntimeConfig::MetadataBytes();
    const bool ready=operation();
    if(RuntimeConfig::MetadataBytes()==before)PluginLoader::ReleaseExtraMemory(charge);
    else metadata_bytes+=charge;
    return ready;
}
template<class Store>bool ensure(Store& store,size_t index){
    if(index<store.Capacity())return true;
    return admit_metadata(sizeof(typename Store::Page),[&](){return store.Ensure(index);});
}
Owner* owner(void* token){
    if(!token)return nullptr;
    for(auto& o:owners)if(o.token==token)return &o;
    for(auto& o:owners)if(!o.token){o.token=token;return &o;}
    const auto next=owners.Capacity();
    if(!ensure(owners,next))return nullptr;
    owners[next].token=token;return &owners[next];
}
void fail(Owner* o){if(o&&o->failures!=UINT64_MAX)++o->failures;}
void release_fail(Owner* o){if(o&&o->release_failures!=UINT64_MAX)++o->release_failures;}
void peak(Owner& o){if(o.requested>o.peak_requested)o.peak_requested=o.requested;if(o.charged>o.peak_charged)o.peak_charged=o.charged;}
uint32_t hash(void* token,uintptr_t address){return uint32_t(((address>>4)^(address>>19)^(reinterpret_cast<uintptr_t>(token)>>5))*0x9e3779b1U)&uint32_t(lookup_capacity-1);}
uint32_t find(void* token,const void* p){
    const auto address=reinterpret_cast<uintptr_t>(p);const auto start=hash(token,address);
    for(size_t n=0;n<lookup_capacity;++n){const auto value=lookup[(size_t(start)+n)&(lookup_capacity-1)];if(!value)return End;if(value!=End&&blocks[value-1].owner==token&&blocks[value-1].address==address)return value-1;}
    return End;
}
void index(uint32_t i){
    auto& b=blocks[i];const auto start=hash(b.owner,b.address);
    for(size_t n=0;n<lookup_capacity;++n){const auto at=(size_t(start)+n)&(lookup_capacity-1);if(!lookup[at]||lookup[at]==End){if(lookup[at]==End)--tombstones;lookup[at]=i+1;b.hash_slot=uint32_t(at);return;}}
}
void rebuild_index(){
    for(size_t i=0;i<lookup_capacity;++i)lookup[i]=0;
    tombstones=0;
    for(uint32_t i=0;i<highwater;++i)if(blocks[i].owner)index(i);
}
bool prepare_index(){
    if(tombstones>lookup_capacity/4)rebuild_index();
    if(size_t(live_blocks)+1<=lookup_capacity-lookup_capacity/4)return true;
    if(lookup_capacity>=UINT64_C(0x100000000))return false;
    const size_t next_capacity=lookup_capacity*2;
    if(next_capacity>SIZE_MAX/sizeof(uint32_t))return false;
    uint32_t* next=nullptr;
    if(!admit_metadata(next_capacity*sizeof(uint32_t),[&](){
        next=static_cast<uint32_t*>(RuntimeConfig::AllocateMetadata(next_capacity*sizeof(uint32_t)));
        return next!=nullptr;
    }))return false;
    // Metadata mappings remain resident. Retired lookup tables stay accounted
    // while the new table is published and every block's hash slot is rebuilt.
    lookup=next;lookup_capacity=next_capacity;rebuild_index();return true;
}
size_t occupied_size(size_t bytes){return (bytes+15)&~size_t(15);}
bool gap(const Arena& a,size_t bytes,size_t alignment,uintptr_t& result,uint32_t& previous,uint32_t& next){
    if(occupied_size(bytes)>a.bytes-a.occupied)return false;
    if(a.tail!=End){
        const auto& tail=blocks[a.tail];const uintptr_t at=(tail.address+tail.bytes+alignment-1)&~uintptr_t(alignment-1);
        if(at<=a.base+a.bytes&&bytes<=a.base+a.bytes-at){result=at;previous=a.tail;next=End;return true;}
    }
    uintptr_t at=a.base;previous=End;next=a.head;
    for(uint64_t steps=0;steps<=highwater;++steps){
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
    if(alignment<16)alignment=16;
    const uint32_t slot=free_head!=End?free_head:highwater;
    // Lookup values reserve zero and UINT32_MAX, so the encoded block index
    // cannot reach UINT32_MAX-1. Admission is checked before publication.
    if(slot>=End-1||!ensure(blocks,slot)||!prepare_index()){fail(&o);return nullptr;}
    uint32_t ai=End,previous=End,next=End;uintptr_t address=0;
    for(size_t i=0;i<arenas.Capacity()&&i<End;++i)if(arenas[i].owner==o.token&&!arenas[i].pages_released&&!arenas[i].release_failed&&gap(arenas[i],bytes,alignment,address,previous,next)){ai=uint32_t(i);break;}
    if(ai==End){
        for(size_t i=0;i<arenas.Capacity()&&i<End;++i)if(!arenas[i].owner){ai=uint32_t(i);break;}
        if(ai==End){
            const auto next_arena=arenas.Capacity();
            if(next_arena>=End||!ensure(arenas,next_arena)){fail(&o);return nullptr;}
            ai=uint32_t(next_arena);
        }
        size_t capacity=(bytes+4095)&~size_t(4095);if(capacity<PoolBytes)capacity=PoolBytes;
        if(!PluginLoader::ReserveExtraMemory(capacity)){fail(&o);return nullptr;}
        uintptr_t at=0;const auto result=sdk_call<uint32_t(*)(uintptr_t*,size_t)>(binding::allocate_pages)(&at,capacity);
        if(result||!at){PluginLoader::ReleaseExtraMemory(capacity);fail(&o);return nullptr;}
        // The reviewed allocator guarantees a page-aligned mapped region on
        // success. Retain an unexpected nonzero allocation rather than forget it.
        arenas[ai]={o.token,at,capacity,0,End,End,false,false};o.charged+=capacity;++o.pools;
        if((at&4095)||at>UINTPTR_MAX-capacity||!Memory::IsWritable(at,capacity)){
            arenas[ai].release_failed=true;release_fail(&o);fail(&o);peak(o);return nullptr;
        }
        address=at;previous=next=End;
    }
    if(slot==free_head)free_head=blocks[slot].next;else ++highwater;
    blocks[slot]={o.token,address,bytes,alignment,ai,next,previous,0,false};o.requested+=bytes;++o.live;
    ++live_blocks;arenas[ai].occupied+=occupied_size(bytes);
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
    lookup[b.hash_slot]=End;++tombstones;--live_blocks;a.occupied-=occupied_size(b.bytes);
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
bool PluginHeap::Free(void* token,void* p){Lock lock;auto* o=owner(token);if(!lock.owned||!o){fail(o);return false;}if(!p)return true;const auto i=find(token,p);if(i==End||blocks[i].resource_pinned){fail(o);return false;}free_block(i);return true;}
size_t PluginHeap::Size(void* token,const void* p){Lock lock;if(!lock.owned||!p)return 0;const auto i=find(token,p);return i==End?0:blocks[i].bytes;}
void* PluginHeap::Reallocate(void* token,void* p,size_t bytes){
    Lock lock;auto* o=owner(token);if(!lock.owned||!o){fail(o);return nullptr;}
    if(!p)return allocate(*o,bytes,16,0);
    const auto i=find(token,p);if(i==End){fail(o);return nullptr;}
    if(blocks[i].resource_pinned){fail(o);return nullptr;}
    if(!bytes){free_block(i);return nullptr;}
    auto& b=blocks[i];
    if(bytes<=b.bytes){o->requested-=b.bytes-bytes;arenas[b.arena].occupied-=occupied_size(b.bytes)-occupied_size(bytes);b.bytes=bytes;return p;}
    auto& a=arenas[b.arena];const auto limit=b.next==End?a.base+a.bytes:blocks[b.next].address;
    if(bytes<=limit-b.address){o->requested+=bytes-b.bytes;a.occupied+=occupied_size(bytes)-occupied_size(b.bytes);b.bytes=bytes;peak(*o);return p;}
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
bool PluginHeap::Discard(void* token){Lock lock;auto* o=owner(token);if(!lock.owned||!o||PluginResources::Count(token))return false;for(uint32_t i=0;i<highwater;++i)if(blocks[i].owner==token)free_block(i);return trim(*o);}
size_t PluginHeap::MetadataBytes(){return metadata_bytes;}
bool PluginHeap::ResourcePinned(void* token,const void* pointer){
    Lock lock;if(!lock.owned||!pointer)return false;
    const auto i=find(token,pointer);return i!=End&&blocks[i].resource_pinned;
}
bool PluginHeap::SetResourcePin(void* token,const void* pointer,bool pinned){
    Lock lock;if(!lock.owned||!pointer)return false;
    const auto i=find(token,pointer);
    if(i==End||blocks[i].resource_pinned==pinned)return false;
    blocks[i].resource_pinned=pinned;return true;
}
size_t PluginHeap::BlockCapacity(){return blocks.Capacity();}
size_t PluginHeap::ArenaCapacity(){return arenas.Capacity();}
size_t PluginHeap::LookupCapacity(){return lookup_capacity;}
}
