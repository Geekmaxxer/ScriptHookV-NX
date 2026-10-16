#include "plugin.h"
#include "natives.h"
namespace {
constexpr uint64_t example_hash=0x5348564558414d31ULL;
struct Context{uint64_t* result;uint32_t count,pad;uint64_t* args;};
void example_native(void* raw){auto& c=*static_cast<Context*>(raw);*c.result=0x1234;}
bool initialize(const ShvNxApi* api){
    if(!shvBindApi(api)||!registerCustomNative(example_hash,example_native))return false;
    const uint32_t marker=1;return scriptSaveFile("initialized.bin",&marker,sizeof(marker));
}
void run(){
    using namespace ScriptHook::Natives;
    uint32_t passed=0,failed=0;
    auto check=[&](bool ok){if(ok)++passed;else ++failed;};
    const auto frame=uint32_t(MISC::GET_FRAME_COUNT());
    check(getNativeCallStatus()==NativeCall_Ok);
    volatile uint64_t local=0x12345678abcdef00ULL;
    scriptWait(0);
    check(uint32_t(MISC::GET_FRAME_COUNT())!=frame&&local==0x12345678abcdef00ULL);
    check(uint32_t(MISC::GET_HASH_KEY("adder"))==0xb779a091u&&getNativeCallStatus()==NativeCall_Ok);
    check(ScriptHook::invoke<uint32_t>(example_hash)==0x1234&&getNativeCallStatus()==NativeCall_Ok);
    const auto timer=MISC::GET_GAME_TIMER();scriptWait(100);
    check(MISC::GET_GAME_TIMER()>=timer&&local==0x12345678abcdef00ULL);
    const uint32_t record[]={0x53485650,1,passed,failed};
    check(scriptSaveFile("settings.bin",record,sizeof(record)));
    uint32_t restored[4]{};size_t got=0;
    check(scriptReadFile("settings.bin",restored,sizeof(restored),&got)&&got==sizeof(record)&&restored[0]==record[0]);
    const char* report=failed?"ScriptHookV-NX external plugin sample\npassed=0\nfailed=1\n":"ScriptHookV-NX external plugin sample\npassed=7\nfailed=0\n";
    size_t n=0;while(report[n])++n;(void)scriptSaveFile("sample-result.txt",report,n);
}
const ShvNxPlugin descriptor={sizeof(ShvNxPlugin),1,1,1,"public-sample",initialize,run};
}
extern "C" SHV_NX_API const ShvNxPlugin* shvPluginQuery(){return &descriptor;}
