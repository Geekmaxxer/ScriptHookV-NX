#pragma once
#include "natives.h"

namespace ScriptHook { namespace Input {
// Nintendo labels. Frontend X (203) is the left face button, Nintendo Y,
// confirmed by the Footprints console test. These are GTA control actions,
// not HID bit masks or PC virtual-key codes.
enum class Button : int32_t { Down=187, Up=188, Left=189, Right=190, Accept=201, Cancel=202, Y=203, X=204, L=205, R=206 };
inline bool Held(Button button) {
    const auto action=static_cast<int32_t>(button);
    return Natives::PAD::IS_CONTROL_PRESSED(2,action)||Natives::PAD::IS_DISABLED_CONTROL_PRESSED(2,action);
}
class ShoulderChord {
    Button trigger;
    bool down=false;
public:
    explicit ShoulderChord(Button button=Button::Y):trigger(button){}
    bool Poll() {
        const bool shoulders=Held(Button::L)&&Held(Button::R);
        const bool pressed=shoulders&&Held(trigger);
        if(shoulders){
            const int32_t actions[]={27,37,44,68,76,85};
            for(auto action:actions)Natives::PAD::DISABLE_CONTROL_ACTION(0,action,true);
            Natives::PAD::DISABLE_CONTROL_ACTION(2,static_cast<int32_t>(trigger),true);
        }
        const bool edge=pressed&&!down;down=pressed;return edge;
    }
    void Reset(){down=false;}
};
} }
