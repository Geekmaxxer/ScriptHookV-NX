#include "script.h"
#include "plugin_manager.h"
#include "mod_support.hpp"

namespace {
using namespace ScriptHook::Plugins;
using namespace Showcase;
// Parsed from CONTROL_ACTION in target dev_ng/core/common/native/commands_pad.sch.
constexpr int Down=187,Up=188,Left=189,Right=190,Accept=201,Cancel=202,X=203,Y=204,L=205,R=206;
constexpr uint32_t maximum=16,visibleRows=8;
struct Row { char name[64],status[96]; Info info; Policy policy; };
Row rows[maximum];uint32_t count,selected;
Policy staged=Policy::Auto;
bool opened;
uint32_t nextRefresh;
char notice[128]{};

void Copy(char* to,size_t capacity,const char* from) {
    size_t at=0;if(from)while(from[at]&&at+1<capacity){to[at]=from[at];++at;}
    to[at]=0;
}
bool Just(int action) {
    return PAD::IS_CONTROL_JUST_PRESSED(2,action)||PAD::IS_DISABLED_CONTROL_JUST_PRESSED(2,action);
}
const char* PolicyText(Policy value) {
    return value==Policy::Auto?"Auto":value==Policy::Manual?"Manual":"Disabled";
}
const char* StateText(State value) {
    switch(value) {
        case State::Disabled:return "DISABLED";
        case State::Resident:return "LOADED";
        case State::Error:return "ERROR";
        case State::Queued:return "QUEUED";
        case State::ResidentPolicyChanged:return "RESTART";
        case State::Stopped:return "STOPPED";
        case State::Finished:return "FINISHED";
        default:return "READY";
    }
}
void Refresh(bool selectPolicy) {
    count=Count();if(count>maximum)count=maximum;
    if(!count)selected=0;else if(selected>=count)selected=count-1;
    for(uint32_t i=0;i<count;++i) {
        Copy(rows[i].name,sizeof(rows[i].name),Filename(i));
        Copy(rows[i].status,sizeof(rows[i].status),StatusText(i));
        if(!GetInfo(i,rows[i].info))rows[i].info={State::Error,0,0};
        rows[i].policy=GetPolicy(i);
    }
    if(count&&selectPolicy)staged=rows[selected].policy;
    nextRefresh=Now()+250;
}
void Result(bool good,const char* success) {
    if(good)Copy(notice,sizeof(notice),success);
    else {TextLine line;line.Add("Action failed (code ").Number(LastActionError()).Add(")");Copy(notice,sizeof(notice),line.CStr());}
    Refresh(true);
}
void Draw() {
    constexpr float x=.025f,y=.12f,width=.47f,row=.042f;
    DrawLine(x,y,width,"ScriptHookV-NX Plugins",{110,225,180});
    const uint32_t top=selected>=visibleRows?selected-visibleRows+1:0;
    for(uint32_t line=0;line<visibleRows;++line) {
        const uint32_t index=top+line;TextLine label;
        if(index<count){
            char shortName[33]{};Copy(shortName,sizeof(shortName),rows[index].name);
            if(rows[index].name[32]){shortName[29]='.';shortName[30]='.';shortName[31]='.';}
            label.Add(index==selected?"> ":"  ").Add(shortName).Add(" | ").Add(StateText(rows[index].info.state));
        }
        else if(!count&&!line)label.Add("No plugins found. Press Y to rescan.");
        DrawLine(x,y+row*(line+1),width,label.CStr(),index==selected?Color{110,225,180}:Color{55,80,95});
    }
    TextLine policy;policy.Add("Next launch: < ").Add(PolicyText(staged)).Add(" >  | A: Save");
    DrawLine(x,y+row*9,width,policy.CStr());
    DrawLine(x,y+row*10,width,"X: Load now  |  Y: Rescan  |  B: Close");
    if(notice[0])DrawLine(x,y+row*11,width,notice);
    else if(count)DrawLine(x,y+row*11,width,rows[selected].status);
}
void ScriptMain() {
    if(!Available()) {SaveStatus("manager-core-unavailable");return;}
    opened=false;selected=0;notice[0]=0;nextRefresh=0;
    Refresh(true);SaveStatus("ready-hold-L-R-DpadDown");
    for(;;) {
        if(!Visible()) {opened=false;WAIT(100);continue;}
        const bool chord=Pressed(L)&&Pressed(R);
        if(chord) {
            const int actions[]={27,37,44,68,76,85,233};
            for(int action:actions)PAD::DISABLE_CONTROL_ACTION(0,action,true);
            PAD::DISABLE_CONTROL_ACTION(2,Down,true);
            if(Just(Down)) {opened=!opened;if(opened){notice[0]=0;Refresh(true);}}
        }
        if(opened) {
            PAD::DISABLE_ALL_CONTROL_ACTIONS(0);
            if(Just(Cancel))opened=false;
            else {
                if(count&&Just(Up)){selected=selected?selected-1:count-1;staged=rows[selected].policy;notice[0]=0;}
                if(count&&Just(Down)&&!chord){selected=(selected+1)%count;staged=rows[selected].policy;notice[0]=0;}
                if(count&&Just(Left))staged=Policy((uint32_t(staged)+2)%3);
                if(count&&Just(Right))staged=Policy((uint32_t(staged)+1)%3);
                if(count&&Just(Accept))Result(SetPolicy(selected,staged),"Startup policy saved. Loaded code stays until exit.");
                if(count&&Just(X))Result(LoadNow(selected),"Load queued. Resident plugins are not loaded twice.");
                if(Just(Y))Result(Rescan(),"Rescan queued. Existing plugins remain resident.");
                if(Due(Now(),nextRefresh))Refresh(false);
                Draw();
            }
        }
        WAIT(0);
    }
}
}
SHV_NX_PLUGIN("plugin-manager",ScriptMain)
