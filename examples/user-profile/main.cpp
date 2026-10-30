#include "script.h"
namespace {
bool Initialize() { return scriptProfileAvailable(); }
void Main() {
    for (unsigned attempt = 0; attempt < 30; ++attempt) {
        ShvNxUserProfile profile{};
        const auto status = scriptGetUserProfile(&profile);
        if (status == ShvNxProfile_Ok) {
            size_t bytes = 0;
            while (bytes < SHV_NX_PROFILE_NAME_BYTES && profile.name[bytes]) ++bytes;
            (void)scriptSaveFile("profile-name.txt", profile.name, bytes);
            return;
        }
        if (status != ShvNxProfile_NotInitialized && status != ShvNxProfile_NoOpenUser) return;
        WAIT(1000);
    }
}
}
SHV_NX_PLUGIN_WITH_INIT("user-profile-example", Main, Initialize)
