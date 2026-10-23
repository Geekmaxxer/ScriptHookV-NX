#pragma once
#include "ui_model.h"
#include "input.h"

namespace ScriptHook { namespace UI {
class Controller {
    Repeat repeats[4]{};bool accept=false,back=false;
    ::ScriptHook::Input::ShoulderChord chord;
    static bool Held(::ScriptHook::Input::Button b){return ::ScriptHook::Input::Held(b);}
public:
    explicit Controller(::ScriptHook::Input::Button toggle=::ScriptHook::Input::Button::Left):chord(toggle){}
    bool TogglePressed() {
        if(!chord.Poll())return false;
        Reset();return true;
    }
    void Reset(){for(auto& r:repeats)r.Reset();accept=Held(::ScriptHook::Input::Button::Accept);back=Held(::ScriptHook::Input::Button::Cancel);}
    UI::Input Poll(uint32_t now) {
        using Button=::ScriptHook::Input::Button;
        const bool shoulders=Held(Button::L)&&Held(Button::R);
        UI::Input keys{};keys.up=repeats[0].Poll(Held(Button::Up)&&!shoulders,now);keys.down=repeats[1].Poll(Held(Button::Down)&&!shoulders,now);
        keys.left=repeats[2].Poll(Held(Button::Left)&&!shoulders,now);keys.right=repeats[3].Poll(Held(Button::Right)&&!shoulders,now);
        bool a=Held(Button::Accept),b=Held(Button::Cancel);keys.accept=a&&!accept;keys.back=b&&!back;accept=a;back=b;
        Natives::PAD::DISABLE_ALL_CONTROL_ACTIONS(0);
        const int32_t actions[]={187,188,189,190,201,202};
        for(int32_t action:actions)Natives::PAD::DISABLE_CONTROL_ACTION(2,action,true);
        return keys;
    }
};
struct Colour { int32_t r=235,g=238,b=242,a=255; };
struct Style { float x=.055f,y=.10f,width=.38f;Colour accent{80,210,180,255}; };
class Renderer {
    struct Cache {char source[96]{},fitted[96]{};float width=-1,scale=-1;};
    Cache cells[22]{};char descriptionSource[320]{},description[320]{};float descriptionWidth=-1;int32_t descriptionLines=1,descriptionLimit=0;
    static void Format(float scale,Colour c,bool right=false,float left=0,float end=1) {
        Natives::HUD::SET_TEXT_FONT(0);Natives::HUD::SET_TEXT_SCALE(scale,scale);
        Natives::HUD::SET_TEXT_COLOUR(c.r,c.g,c.b,c.a);Natives::HUD::SET_TEXT_CENTRE(false);
        Natives::HUD::SET_TEXT_RIGHT_JUSTIFY(right);Natives::HUD::SET_TEXT_WRAP(left,end);
    }
    static float Measure(const char* text,float scale) {
        Format(scale,{});Natives::HUD::BEGIN_TEXT_COMMAND_GET_SCREEN_WIDTH_OF_DISPLAY_TEXT("STRING");
        Util::AppendHudText(text);return Natives::HUD::END_TEXT_COMMAND_GET_SCREEN_WIDTH_OF_DISPLAY_TEXT(true);
    }
    static void Text(const char* text,float x,float y,float scale,Colour c,bool right=false,float left=0,float end=1) {
        Format(scale,c,right,left,end);Natives::HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");Util::AppendHudText(text);
        Natives::HUD::END_TEXT_COMMAND_DISPLAY_TEXT(x,y,0);
    }
    static void Rect(float x,float y,float w,float h,Colour c) {Natives::GRAPHICS::DRAW_RECT(x+w*.5f,y+h*.5f,w,h,c.r,c.g,c.b,c.a,false);}
    const char* Fit(size_t index,const char* text,float width,float scale) {
        Cache& cache=cells[index];if(Same(cache.source,text)&&cache.width==width&&cache.scale==scale)return cache.fitted;
        Copy(cache.source,text);Copy(cache.fitted,text);cache.width=width;cache.scale=scale;
        if(Measure(cache.fitted,scale)<=width)return cache.fitted;
        size_t length=0;while(cache.source[length])++length;
        size_t low=0,high=length,best=0;
        while(low<=high) {
            const size_t middle=low+(high-low)/2;size_t cut=middle;
            while(cut&&(uint8_t(cache.source[cut])&0xc0)==0x80)--cut;
            if(cut+4>sizeof(cache.fitted))cut=sizeof(cache.fitted)-4;
            while(cut&&(uint8_t(cache.source[cut])&0xc0)==0x80)--cut;
            for(size_t i=0;i<cut;++i)cache.fitted[i]=cache.source[i];
            cache.fitted[cut]='.';cache.fitted[cut+1]='.';cache.fitted[cut+2]='.';cache.fitted[cut+3]=0;
            if(Measure(cache.fitted,scale)<=width){best=cut;low=middle+1;}else{if(middle==0)break;high=middle-1;}
        }
        for(size_t i=0;i<best;++i)cache.fitted[i]=cache.source[i];
        cache.fitted[best]='.';cache.fitted[best+1]='.';cache.fitted[best+2]='.';cache.fitted[best+3]=0;
        return cache.fitted;
    }
public:
    template<size_t R,size_t P> void Draw(const Menu<R,P>& menu,Style style={}) {
        if(!menu.IsOpen()||menu.PageChanged()||Natives::HUD::IS_PAUSE_MENU_ACTIVE())return;
        if(!(style.width>=.28f&&style.width<=.6f&&style.x>=.02f&&style.x+style.width<=.98f&&style.y>=.02f&&style.y<=.15f))style=Style{};
        const float x=style.x,y=style.y,w=style.width,rowHeight=.035f,padding=.012f;
        Rect(x,y,w,.052f,{19,27,37,245});Rect(x,y,w,.003f,style.accent);
        Text(Fit(20,menu.Title(),w-padding*2,.40f),x+padding,y+.008f,.40f,{});
        const size_t shown=menu.Count()<menu.VisibleRows()?menu.Count():menu.VisibleRows();
        const float start=y+.052f;
        for(size_t line=0;line<shown;++line) {
            size_t index=menu.Top()+line;if(index>=menu.Count())break;const Row& r=menu.At(index);const bool selected=index==menu.Selected();
            const float at=start+float(line)*rowHeight;
            Rect(x,at,w,rowHeight,selected?Colour{37,64,72,250}:Colour{17,24,33,235});
            if(selected)Rect(x,at,.003f,rowHeight,style.accent);
            Colour c=r.enabled?Colour{}:Colour{140,149,161,255};
            Text(Fit(line*2,r.label,w*.54f,.32f),x+padding,at+.004f,.32f,c);
            Text(Fit(line*2+1,r.value,w*.39f,.30f),x+w-padding,at+.005f,.30f,selected&&r.enabled?style.accent:c,true,x,x+w-padding);
        }
        const float footer=start+float(shown)*rowHeight;
        Rect(x,footer,w,.032f,{19,27,37,245});
        Util::TextBuffer<96> status;status.Add(menu.Valid()?"A: Select   B: Back   ":"Menu unavailable   ");
        status.UInt(menu.Count()?uint32_t(menu.Selected()+1):0).Add(" / ").UInt(uint32_t(menu.Count()));
        Text(Fit(21,status.CStr(),w-padding*2,.27f),x+padding,footer+.004f,.27f,{173,187,199,255});
        const char* desc=menu.Count()?menu.At(menu.Selected()).description:"";
        if(*desc) {
            int32_t limit=int32_t((.95f-footer-.058f)/.025f);if(limit<1)limit=1;
            if(!Same(descriptionSource,desc)||descriptionWidth!=w||descriptionLimit!=limit) {
                Copy(descriptionSource,desc);Copy(description,desc);descriptionWidth=w;descriptionLimit=limit;
                auto lines=[&]() {
                    Format(.29f,{},false,x+padding,x+w-padding);
                    Natives::HUD::BEGIN_TEXT_COMMAND_GET_NUMBER_OF_LINES_FOR_STRING("STRING");Util::AppendHudText(description);
                    int32_t n=Natives::HUD::END_TEXT_COMMAND_GET_NUMBER_OF_LINES_FOR_STRING(x+padding,footer+.048f);return n<1?1:n;
                };
                descriptionLines=lines();
                if(descriptionLines>limit) {
                    size_t length=0;while(descriptionSource[length])++length;
                    size_t low=0,high=length<316?length:316,best=0;
                    auto prefix=[&](size_t cut){while(cut&&(uint8_t(descriptionSource[cut])&0xc0)==0x80)--cut;for(size_t i=0;i<cut;++i)description[i]=descriptionSource[i];description[cut]='.';description[cut+1]='.';description[cut+2]='.';description[cut+3]=0;return cut;};
                    while(low<=high){size_t middle=low+(high-low)/2,cut=prefix(middle);if(lines()<=limit){best=cut;low=middle+1;}else{if(!middle)break;high=middle-1;}}
                    prefix(best);descriptionLines=lines();
                }
            }
            const float height=float(descriptionLines)*.025f+.02f;
            Rect(x,footer+.038f,w,height,{17,24,33,240});
            Text(description,x+padding,footer+.046f,.29f,{},false,x+padding,x+w-padding);
        }
    }
};
static_assert(sizeof(Menu<>)+sizeof(Renderer)<32*1024,"Default UI objects must fit within half a standard script stack");
} }
