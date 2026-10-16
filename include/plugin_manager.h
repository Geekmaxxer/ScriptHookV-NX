#pragma once
#include "nativeCaller.h"

// Optional manager API. Plugins are independent of any particular NSC menu.
// Policy changes rename the NRO and take effect at the next game launch.
// LoadNow queues work at a safe script boundary; it never unloads code.
namespace ScriptHook { namespace Plugins {
enum class Policy : uint32_t { Disabled = 0, Manual = 1, Auto = 2 };
enum class State : uint32_t {
    Disabled = 0, Resident = 1, Error = 2, Queued = 3, Ready = 4,
    ResidentPolicyChanged = 5, Stopped = 6, Finished = 7
};
struct Info { State state; uint32_t memoryKiB, step; };
inline constexpr uint64_t VersionHash = 0x5348564e58505631ULL;
inline bool Available() {
    return nativeIsAvailable(VersionHash) && invoke<uint32_t>(VersionHash) >= 2;
}
inline uint32_t Count() { return invoke<uint32_t>(0x5348564e58504331ULL); }
inline const char* Filename(uint32_t index) { return invoke<const char*>(0x5348564e58504e31ULL,index); }
inline const char* Id(uint32_t index) { return invoke<const char*>(0x5348564e58504931ULL,index); }
inline const char* StatusText(uint32_t index) { return invoke<const char*>(0x5348564e58505431ULL,index); }
inline bool GetInfo(uint32_t index, Info& out) {
    // NSC INT output slots are eight bytes. Use matching storage, then narrow
    // to the C++ view after the bridge has validated all three destinations.
    uint64_t state = 0, memory = 0, step = 0;
    if (!invoke<bool>(0x5348564e58505331ULL,index,&state,&memory,&step)) return false;
    out = {State(uint32_t(state)),uint32_t(memory),uint32_t(step)}; return true;
}
inline Policy GetPolicy(uint32_t index) { return Policy(invoke<uint32_t>(0x5348564e58505031ULL,index)); }
inline bool SetPolicy(uint32_t index,Policy policy) { return invoke<bool>(0x5348564e58505032ULL,index,uint32_t(policy)); }
inline bool LoadNow(uint32_t index) { return invoke<bool>(0x5348564e58504c31ULL,index); }
inline bool ProcessControlsAvailable() {
    return nativeIsAvailable(0x5348564e58504b31ULL) && nativeIsAvailable(0x5348564e58505231ULL);
}
// Cancellation is cooperative at the next scheduler boundary. This preserves
// module mappings/custom-native handlers and does not change startup policy.
inline bool KillProcess(uint32_t index) { return ProcessControlsAvailable() && invoke<bool>(0x5348564e58504b31ULL,index); }
inline bool IsRunning(uint32_t index) { return ProcessControlsAvailable() && invoke<bool>(0x5348564e58505231ULL,index); }
inline bool Rescan() { return invoke<bool>(0x5348564e5850524cULL); }
inline uint32_t LastActionError() { return invoke<uint32_t>(0x5348564e58504531ULL); }
} }
