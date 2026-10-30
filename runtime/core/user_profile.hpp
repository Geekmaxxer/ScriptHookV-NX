#pragma once
#include "plugin_profile.h"
namespace ScriptHook {
class UserProfile {
public:
    static ShvNxProfileStatus Read(ShvNxUserProfile& profile);
};
}
