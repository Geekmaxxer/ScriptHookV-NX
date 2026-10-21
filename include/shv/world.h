#pragma once
#include "nativeCaller.h"

namespace ScriptHook { namespace World {
// Audited target native. Its scanner output is ordered by proximity; this is
// not a replacement for PC world-pool enumeration.
inline constexpr uint64_t NearbyPedsNative = 0x46951d3186547c7aULL;

template<size_t Capacity = 32> class NearbyPeds {
    static_assert(Capacity >= 1 && Capacity <= 64, "Nearby-ped capacity must be 1-64");
    struct alignas(8) Slot { int32_t value; uint32_t padding; };
    struct Buffer { Slot capacity; Slot entries[Capacity]; };
    static_assert(sizeof(Slot) == 8 && offsetof(Buffer, entries) == 8);
    static_assert(sizeof(Buffer) == (Capacity + 1) * 8);
    Buffer buffer{};
    uint32_t count = 0;
public:
    // The native consumes a VM array: one eight-byte length slot, then
    // eight-byte slots whose low 32 bits hold handles. A plain Ped[] is unsafe.
    // Passing Buffer* also makes the caller validate the complete output range.
    bool Refresh(Ped anchor, int32_t excludedPedType = -1) {
        Clear();
        if (!anchor) return false;
        buffer.capacity.value = static_cast<int32_t>(Capacity);
        const int32_t written = invoke<int32_t>(NearbyPedsNative, anchor, &buffer, excludedPedType);
        if (getNativeCallStatus() != NativeCall_Ok || buffer.capacity.value != static_cast<int32_t>(Capacity) ||
            written < 0 || written > static_cast<int32_t>(Capacity)) {
            Clear();
            return false;
        }
        count = static_cast<uint32_t>(written);
        return true;
    }
    void Clear() { buffer = {}; count = 0; }
    uint32_t Size() const { return count; }
    Ped operator[](uint32_t index) const { return index < count ? buffer.entries[index].value : 0; }
};
} }
