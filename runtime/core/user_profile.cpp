#include "user_profile.hpp"
#include "memory.hpp"
#include "storage.hpp"
#include "profile_bindings.hpp"

namespace ScriptHook {
namespace {
struct Uid { uint64_t words[2]; };
struct Nickname { char text[33]; };
static_assert(sizeof(Uid) == 16 && sizeof(Nickname) == 33);
bool verified() {
    if (!Memory::ResolveSdk()) return false;
    for (const auto& proof : profile_binding::proofs) {
        const auto address = Memory::s_sdkBase + proof.offset;
        if (!Memory::IsExecutable(address) || !Memory::IsReadable(address, sizeof(proof.bytes))) return false;
        const auto* actual = reinterpret_cast<const uint8_t*>(address);
        for (size_t i = 0; i < sizeof(proof.bytes); ++i) if (actual[i] != proof.bytes[i]) return false;
    }
    return true;
}
bool valid_name(const char (&name)[33]) {
    size_t at = 0;
    while (at < 32 && name[at]) {
        const auto first = uint8_t(name[at++]);
        if (first < 0x20 || first == 0x7f) return false;
        if (first < 0x80) continue;
        size_t remaining;
        uint32_t value, minimum;
        if (first >= 0xc2 && first <= 0xdf) { remaining = 1; value = first & 31; minimum = 0x80; }
        else if (first >= 0xe0 && first <= 0xef) { remaining = 2; value = first & 15; minimum = 0x800; }
        else if (first >= 0xf0 && first <= 0xf4) { remaining = 3; value = first & 7; minimum = 0x10000; }
        else return false;
        if (remaining > 32 - at) return false;
        while (remaining--) {
            const auto next = uint8_t(name[at++]);
            if ((next & 0xc0) != 0x80) return false;
            value = (value << 6) | (next & 63);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff) ||
            (value >= 0x80 && value <= 0x9f) || value == 0x2028 || value == 0x2029) return false;
    }
    return at && name[32] == 0;
}
}
ShvNxProfileStatus UserProfile::Read(ShvNxUserProfile& out) {
    out = {}; out.size = sizeof(out); out.abi = SHV_NX_PROFILE_ABI_VERSION;
    auto result = [&](ShvNxProfileStatus status) { out.status = status; return status; };
    if (!verified()) return result(ShvNxProfile_UnsupportedTarget);
    if (!sdk_call<bool(*)()>(profile_binding::initialized)()) return result(ShvNxProfile_NotInitialized);
    Uid users[8]{}; int32_t count = 0;
    out.platform_result = sdk_call<uint32_t(*)(int32_t*, Uid*, int32_t)>(profile_binding::open_users)(&count, users, 8);
    if (out.platform_result) return result(ShvNxProfile_PlatformError);
    if (count < 0 || count > 8) return result(ShvNxProfile_PlatformError);
    if (!count) return result(ShvNxProfile_NoOpenUser);
    if (count != 1) return result(ShvNxProfile_AmbiguousUser);
    if (!(users[0].words[0] | users[0].words[1])) return result(ShvNxProfile_NoOpenUser);
    Nickname nickname{};
    out.platform_result = sdk_call<uint32_t(*)(Nickname*, const Uid&)>(profile_binding::nickname)(&nickname, users[0]);
    if (out.platform_result) return result(ShvNxProfile_PlatformError);
    if (!valid_name(nickname.text)) return result(ShvNxProfile_InvalidName);
    for (size_t i = 0; i < sizeof(out.name) && nickname.text[i]; ++i) out.name[i] = nickname.text[i];
    return result(ShvNxProfile_Ok);
}
}
