#include "script.h"
#include "shv/text.h"

void ScriptMain() {
    const uint64_t required[] = {0xf1ec2c71fd1371b8ULL, 0xcdda0c58b818f6b2ULL, 0x76db21247ae4e4e2ULL,
        0x1a53079994915fa6ULL, 0xf68e5437af17efbcULL, 0x3f03c2d4efa888bcULL, 0x71f4002cb1a0b451ULL,
        0x21fc15ae6b6188c4ULL, 0xb9a0cf201cf801f2ULL, 0xe835f806be49c67bULL};
    for (uint64_t hash : required) if (!nativeIsAvailable(hash)) return;
    while (true) {
        if (!HUD::IS_PAUSE_MENU_ACTIVE()) {
            HUD::SET_TEXT_FONT(0); HUD::SET_TEXT_SCALE(0.31f, 0.31f);
            HUD::SET_TEXT_COLOUR(255, 255, 255, 255);
            HUD::SET_TEXT_CENTRE(false); HUD::SET_TEXT_RIGHT_JUSTIFY(false); HUD::SET_TEXT_WRAP(0.02f, 0.40f);
            HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
            (void)ScriptHook::Util::AppendHudText("Hello, Switch modders! This overlay yields every frame and hides while the pause menu is active. Longer plain text is split safely at UTF-8 boundaries.");
            HUD::END_TEXT_COMMAND_DISPLAY_TEXT(0.02f, 0.25f, 0);
        }
        WAIT(0);
    }
}
SHV_NX_PLUGIN("tutorial-overlay", ScriptMain)
