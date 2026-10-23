#pragma once
#include "text.h"

namespace ScriptHook { namespace UI {
struct Input { bool up=false, down=false, left=false, right=false, accept=false, back=false; };
enum class Kind { Action, Toggle, Integer, Choice, Submenu, ReadOnly };
template<size_t N> inline void Copy(char (&out)[N], const char* text) {
    size_t used=0; if(!text)text="";
    while(*text) { size_t n=Util::Utf8Unit(text); if(!n||used+n+1>N)break; for(size_t i=0;i<n;++i)out[used++]=*text++; }
    out[used]=0;
    if(*text&&N>4) { while(used&&used+4>N) { --used; while(used&&(uint8_t(out[used])&0xc0)==0x80)--used; } out[used++]='.';out[used++]='.';out[used++]='.';out[used]=0; }
}
inline bool Same(const char* a,const char* b) { while(*a&&*a==*b){++a;++b;}return *a==*b; }
struct Row {
    uint32_t id=0,target=0;
    Kind kind=Kind::ReadOnly;
    char label[96]{},value[64]{},description[320]{};
    bool enabled=false;
    bool* boolean=nullptr; int32_t* integer=nullptr;
    int32_t minimum=0,maximum=0,step=1;
    const char* const* choices=nullptr;
};
// Immediate-mode rows borrow setting pointers until End(). Text is copied.
// IDs must be stable and nonzero. No heap, callbacks, static constructors or I/O.
template<size_t MaxRows=32,size_t MaxPages=8> class Menu {
    static_assert(MaxRows>0&&MaxRows<=64&&MaxPages>0&&MaxPages<=16,"Bounded UI capacities required");
    struct PageState { uint32_t id=0,selection=0; size_t top=0; };
    PageState pages[MaxPages]{}; size_t history[MaxPages]{},pageCount=1,depth=0;
    Row rows[MaxRows]{}; size_t count=0,selected=0,visible=8;
    Input input{}; bool open=false,valid=true,pageChanged=false;
    uint32_t activated=0,changed=0; char title[96]{};
    Row* Add(uint32_t id,const char* label,Kind kind,const char* description,bool enabled) {
        if(count==MaxRows||id==0){valid=false;return nullptr;}
        for(size_t i=0;i<count;++i)if(rows[i].id==id){valid=false;return nullptr;}
        Row& row=rows[count++];row=Row{};row.id=id;row.kind=kind;row.enabled=enabled;
        Copy(row.label,label);Copy(row.description,description);return &row;
    }
    bool Push(uint32_t id) {
        if(id==0||depth+1>=MaxPages)return false;
        for(size_t i=0;i<=depth;++i)if(pages[history[i]].id==id)return false;
        size_t index=0;for(;index<pageCount;++index)if(pages[index].id==id)break;
        if(index==pageCount){if(pageCount==MaxPages)return false;pages[pageCount++].id=id;}
        history[++depth]=index;return true;
    }
    void Values() {
        for(size_t i=0;i<count;++i) {
            Row& r=rows[i];
            if(r.kind==Kind::Toggle&&r.boolean)Copy(r.value,*r.boolean?"[x]":"[ ]");
            else if(r.kind==Kind::Integer&&r.integer){Util::TextBuffer<64> text;text.Add("< ").Int(*r.integer).Add(" >");Copy(r.value,text.CStr());}
            else if(r.kind==Kind::Choice&&r.integer&&r.choices&&*r.integer>=r.minimum&&*r.integer<=r.maximum){Util::TextBuffer<64> text;text.Add("< ").Add(r.choices[*r.integer]).Add(" >");Copy(r.value,text.CStr());}
            else if(r.kind==Kind::Submenu)Copy(r.value,">");
        }
    }
public:
    bool IsOpen()const{return open;}
    void Open(){open=true;}
    void Close(){open=false;}
    void Toggle(){open=!open;}
    uint32_t Page()const{return pages[history[depth]].id;}
    uint32_t Activated()const{return activated;}
    uint32_t Changed()const{return changed;}
    bool Valid()const{return valid;}
    bool PageChanged()const{return pageChanged;}
    size_t Count()const{return count;}
    size_t Selected()const{return selected;}
    size_t Top()const{return pages[history[depth]].top;}
    size_t VisibleRows()const{return visible;}
    const Row& At(size_t i)const{return rows[i<count?i:0];}
    const char* Title()const{return title;}
    void Begin(const char* name,Input keys={},size_t visibleRows=8) {
        count=0;activated=changed=0;valid=true;pageChanged=false;input=keys;
        visible=visibleRows<1?1:visibleRows>10?10:visibleRows;Copy(title,name);
    }
    void Action(uint32_t id,const char* label,const char* description="",bool enabled=true){Add(id,label,Kind::Action,description,enabled);}
    void Toggle(uint32_t id,const char* label,bool& value,const char* description="",bool enabled=true){Row* r=Add(id,label,Kind::Toggle,description,enabled);if(r)r->boolean=&value;}
    void Integer(uint32_t id,const char* label,int32_t& value,int32_t minimum,int32_t maximum,int32_t step=1,const char* description="",bool enabled=true) {
        Row* r=Add(id,label,Kind::Integer,description,enabled);if(!r)return;
        if(minimum>maximum||step<=0){valid=false;r->enabled=false;return;}
        r->integer=&value;r->minimum=minimum;r->maximum=maximum;r->step=step;
        if(value<minimum)value=minimum;
        if(value>maximum)value=maximum;
    }
    void Choice(uint32_t id,const char* label,int32_t& value,const char* const* choices,size_t choicesCount,const char* description="",bool enabled=true) {
        if(!choices||choicesCount==0||choicesCount>1024){valid=false;return;}
        Row* r=Add(id,label,Kind::Choice,description,enabled);if(!r)return;
        r->integer=&value;r->minimum=0;r->maximum=int32_t(choicesCount-1);r->choices=choices;
        if(value<0)value=0;
        if(value>r->maximum)value=r->maximum;
    }
    void Submenu(uint32_t id,const char* label,uint32_t target,const char* description="",bool enabled=true){Row* r=Add(id,label,Kind::Submenu,description,enabled);if(r)r->target=target;}
    void ReadOnly(uint32_t id,const char* label,const char* value,const char* description=""){Row* r=Add(id,label,Kind::ReadOnly,description,false);if(r)Copy(r->value,value);}
    void End() {
        Values();if(!open||!valid)return;
        if(input.back){if(depth){--depth;pageChanged=true;}else Close();return;}
        if(!count)return;
        PageState& state=pages[history[depth]];
        selected=0;for(size_t i=0;i<count;++i)if(rows[i].id==state.selection){selected=i;break;}
        if(!rows[selected].enabled){size_t i=0;while(i<count&&!rows[i].enabled)++i;if(i==count)return;selected=i;}
        int direction=input.down&&!input.up?1:input.up&&!input.down?-1:0;
        if(direction)for(size_t attempts=0;attempts<count;++attempts){selected=direction>0?(selected+1)%count:(selected?selected-1:count-1);if(rows[selected].enabled)break;}
        state.selection=rows[selected].id;
        if(selected<state.top)state.top=selected;
        if(selected>=state.top+visible)state.top=selected-visible+1;
        if(state.top>=count)state.top=count-1;
        Row& r=rows[selected];
        if(input.accept) {
            if(r.kind==Kind::Toggle&&r.boolean){*r.boolean=!*r.boolean;changed=r.id;}
            else if(r.kind==Kind::Action)activated=r.id;
            else if(r.kind==Kind::Submenu){pageChanged=Push(r.target);if(!pageChanged)valid=false;}
        }
        const int adjust=input.right&&!input.left?1:input.left&&!input.right?-1:0;
        if(adjust&&r.integer&&(r.kind==Kind::Integer||r.kind==Kind::Choice)) {
            const int32_t old=*r.integer;int64_t next=int64_t(old)+int64_t(adjust)*r.step;
            if(r.kind==Kind::Choice){if(next<r.minimum)next=r.maximum;if(next>r.maximum)next=r.minimum;}
            else {if(next<r.minimum)next=r.minimum;if(next>r.maximum)next=r.maximum;}
            *r.integer=int32_t(next);if(old!=*r.integer)changed=r.id;
        }
        Values();
    }
};
// Deterministic repeat: edge, 350 ms initial delay, then 100 ms. No catch-up.
class Repeat {
    bool down=false;uint32_t deadline=0;
public:
    bool Poll(bool held,uint32_t now){if(!held){down=false;return false;}if(!down){down=true;deadline=now+350;return true;}if(int32_t(now-deadline)>=0){deadline=now+100;return true;}return false;}
    void Reset(){down=false;}
};
} }
