#include "script.h"
#include "plugin_manager.h"
#include "shv/input.h"
#include "shv/text.h"
namespace {
using namespace ScriptHook;
uint32_t snapshots;bool saved,available;
void Number(Util::TextBuffer<16384>& text,uint64_t value){
    char reversed[20],digits[21]{};uint32_t n=0,out=0;
    do{reversed[n++]=char('0'+value%10);value/=10;}while(value);
    while(n)digits[out++]=reversed[--n];
    text.Add(digits);
}
void Snapshot(){
    Util::TextBuffer<16384> text;text.Add("ScriptHookV-NX plugin timing snapshot\nunits=microseconds\n");
    text.Add("measurement=elapsed_dispatch_time_including_native_calls_io_and_os_preemption\n");
    if(!available)text.Add("timing_available=0\n");
    else {
        text.Add("timing_available=1\n");
        const char* kinds[]={"script","callback","cleanup","initialize"};
        const auto count=Plugins::Count();
        for(uint32_t i=0;i<count&&i<16;++i){
            text.Add("plugin=").Add(Plugins::Filename(i)).Add("\n");
            Plugins::Info info{};if(Plugins::GetInfo(i,info))text.Add("mapped_accounted_kib=").UInt(info.memoryKiB).Add("\n");
            for(uint32_t kind=0;kind<4;++kind){
                Plugins::Timing timing{};if(!Plugins::GetTiming(i,Plugins::TimingKind(kind),timing))continue;
                text.Add(kinds[kind]).Add("_calls=");Number(text,timing.calls);
                text.Add("\n").Add(kinds[kind]).Add("_total_us=");Number(text,timing.totalMicroseconds);
                text.Add("\n").Add(kinds[kind]).Add("_worst_us=");Number(text,timing.worstMicroseconds);text.Add("\n");
            }
        }
    }
    saved=text.Complete()&&scriptSaveFile("performance-report.txt",text.CStr(),text.Size());++snapshots;
}
void Overlay(){
    if(HUD::IS_PAUSE_MENU_ACTIVE())return;
    HUD::SET_TEXT_FONT(0);HUD::SET_TEXT_SCALE(.28f,.28f);HUD::SET_TEXT_COLOUR(230,245,255,255);
    HUD::SET_TEXT_CENTRE(false);HUD::SET_TEXT_RIGHT_JUSTIFY(false);HUD::SET_TEXT_WRAP(.025f,.75f);
    HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
    Util::TextBuffer<192> text;text.Add(available?"Runtime diagnostics | L + R + X: save snapshot | ":"Runtime diagnostics unavailable | ");
    text.UInt(snapshots).Add(saved?" saved":" save failed");(void)Util::AppendHudText(text.CStr());
    HUD::END_TEXT_COMMAND_DISPLAY_TEXT(.025f,.36f,0);
}
void Main(){
    available=Plugins::TimingAvailable();
    WAIT(2000);Snapshot();if(!available)return;
    Input::ShoulderChord chord(Input::Button::X);
    for(;;){if(chord.Poll())Snapshot();Overlay();WAIT(0);}
}
void Stop(){Snapshot();}
}
SHV_NX_PLUGIN_WITH_STOP("runtime-diagnostics",Main,Stop)
