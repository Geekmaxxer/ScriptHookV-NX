#include "script.h"
#include "shv/managed.h"
#include "shv/text.h"

using namespace ScriptHook;
namespace {
constexpr uint32_t Count=4096;
uint32_t run,failed,completed,verified,peak_allocations;
bool cleanup_complete;
struct State {
    void* blocks[Count]{};
    ~State() noexcept {
        for(auto* pointer:blocks)if(pointer)(void)scriptFree(pointer);
    }
};

void Report() {
    ShvNxMemoryStats stats{};
    const bool available=scriptGetMemoryStats(&stats);
    Util::TextBuffer<320> text;
    text.Add("run=").UInt(run).Add("\nfailed=").UInt(failed)
        .Add("\ncompleted=").UInt(completed).Add("\nverified=").UInt(verified)
        .Add("\npeak_allocations=").UInt(peak_allocations)
        .Add("\nowner_cleanup_complete=").UInt(cleanup_complete)
        .Add("\nowned_allocations=").UInt(available?uint32_t(stats.allocations):UINT32_MAX)
        .Add("\nregistered_resources=").UInt(scriptResourceCount()).Add("\n");
    if(text.Complete())(void)scriptSaveFile("result.txt",text.CStr(),text.Size());
}
void Audit(void*) {
    ShvNxMemoryStats stats{};
    cleanup_complete=scriptGetMemoryStats(&stats)&&!stats.allocations&&!scriptResourceCount()&&scriptTrimMemory();
    if(!cleanup_complete)++failed;
    Report();
}
bool Initialize(){return scriptMemoryAvailable()&&scriptResourcesAvailable();}

void Main() {
    ++run;failed=completed=verified=peak_allocations=0;cleanup_complete=false;
    if(!scriptResourceRegister(Audit,nullptr)){++failed;Report();return;}
    auto state=Memory::Managed<State>::TryMake();
    if(!state){++failed;return;}
    for(uint32_t i=0;i<Count;++i){
        {
            auto* owner=state.Get();
            if(!owner){++failed;return;}
            auto* data=static_cast<uint64_t*>(scriptAllocate(32,16,SHV_NX_MEMORY_ZERO));
            if(!data){++failed;return;}
            owner->blocks[i]=data;data[0]=i+1;data[1]=~uint64_t(i+1);
        }
        if(i%64==63)WAIT(0);
    }
    {
        ShvNxMemoryStats stats{};
        if(!scriptGetMemoryStats(&stats)||stats.allocations!=Count+1){++failed;return;}
        peak_allocations=uint32_t(stats.allocations);
    }
    for(uint32_t i=0;i<Count;++i){
        {
            auto* owner=state.Get();
            if(!owner){++failed;return;}
            const auto* data=static_cast<const uint64_t*>(owner->blocks[i]);
            if(scriptAllocationSize(data)!=32||data[0]!=i+1||data[1]!=~uint64_t(i+1)){++failed;return;}
            ++verified;
        }
        if(i%128==127)WAIT(0);
    }
    for(uint32_t i=0;i<128;++i){
        {
            auto* owner=state.Get();
            if(!owner||!scriptFree(owner->blocks[i])){++failed;return;}
            owner->blocks[i]=nullptr;
        }
        if(i%32==31)WAIT(0);
    }
    for(uint32_t i=0;i<128;++i){
        {
            auto* owner=state.Get();
            if(!owner){++failed;return;}
            auto* data=static_cast<uint64_t*>(scriptAllocate(32,16,0));
            if(!data){++failed;return;}
            owner->blocks[i]=data;data[0]=i+1;data[1]=~uint64_t(i+1);
        }
        if(i%32==31)WAIT(0);
    }
    for(uint32_t i=0;i<Count;++i){
        {
            auto* owner=state.Get();
            if(!owner){++failed;return;}
            const auto* data=static_cast<const uint64_t*>(owner->blocks[i]);
            if(!data||data[0]!=i+1||data[1]!=~uint64_t(i+1)||!scriptFree(owner->blocks[i])){++failed;return;}
            owner->blocks[i]=nullptr;
        }
        if(i%128==127)WAIT(0);
    }
    completed=1;
}
}
SHV_NX_PLUGIN_WITH_INIT("heap-records-example",Main,Initialize)
