#include "plugin.h"
#include "nativeCaller.h"

namespace {
// Demonstration hash only. Assign a distinct hash to every public native.
constexpr uint64_t AddHash = 0x5348564e58414431ULL;
struct Context { uint64_t* result; uint32_t count, padding; uint64_t* args; };
void Add(void* raw) {
    auto& context = *static_cast<Context*>(raw);
    *context.result = 0;
    if (context.count != 2) return;
    // VM INT slots have eight bytes, but INT arithmetic uses their low 32 bits.
    const uint32_t sum = uint32_t(context.args[0]) + uint32_t(context.args[1]);
    *context.result = (sum & 0x80000000u) ? (0xffffffff00000000ULL | sum) : sum;
    // Custom handlers return immediately. Never WAIT or perform slow I/O here.
}
bool Initialize(const ShvNxApi* api) { return shvBindApi(api) && registerCustomNative(AddHash, Add); }
void ScriptMain() {
    const int32_t sum = ScriptHook::invoke<int32_t>(AddHash, int32_t(7), int32_t(-2));
    const char* message = sum == 5 && getNativeCallStatus() == NativeCall_Ok ? "provider=ready\n" : "provider=failed\n";
    size_t n = 0; while (message[n]) ++n;
    (void)scriptSaveFile("provider.txt", message, n);
    // The handler remains registered even after this run entry returns.
}
const ShvNxPlugin descriptor = {sizeof(ShvNxPlugin), SHV_NX_PLUGIN_ABI_VERSION,
    SHV_NX_CALLER_ABI_VERSION, SHV_NX_RUNTIME_ABI_VERSION, "tutorial-provider", Initialize, ScriptMain};
}
extern "C" SHV_NX_API const ShvNxPlugin* shvPluginQuery() { return &descriptor; }
