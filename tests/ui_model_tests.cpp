#include "shv/ui_model.h"
#ifdef SHVNX_FREESTANDING_TEST
extern "C" void* memset(void* target,int value,size_t count){auto p=static_cast<volatile unsigned char*>(target);for(size_t i=0;i<count;++i)p[i]=static_cast<unsigned char>(value);return target;}
extern "C" void* memcpy(void* target,const void* source,size_t count){auto p=static_cast<volatile unsigned char*>(target);auto q=static_cast<const unsigned char*>(source);for(size_t i=0;i<count;++i)p[i]=q[i];return target;}
#endif

extern "C" uint64_t shvnx_ui_model_tests() {
    using namespace ScriptHook::UI;
    uint32_t passed=0,failed=0;
    auto check=[&](bool ok){if(ok)++passed;else ++failed;};
    Menu<16,3> menu;menu.Open();bool on=false;int32_t number=5,choice=0;
    const char* names[]={"First","Second","Third"};
    auto frame=[&](Input input={}) {
        menu.Begin("Test",input,3);
        if(menu.Page()==0){menu.Toggle(1,"Toggle",on);menu.Integer(2,"Value",number,-10,10,5);menu.ReadOnly(3,"Read only","Info");menu.Choice(4,"Choice",choice,names,3);menu.Submenu(5,"Page",1);menu.Action(6,"Action");}
        else {menu.Action(10,"Child action");menu.Submenu(11,"Recursive",1);}
        menu.End();
    };
    frame();check(menu.Count()==6&&menu.Selected()==0&&menu.Valid());
    Input keys{};keys.accept=true;frame(keys);check(on&&menu.Changed()==1&&!menu.Activated());
    keys={};keys.down=true;frame(keys);check(menu.Selected()==1);
    keys={};keys.right=true;frame(keys);check(number==10&&menu.Changed()==2);
    frame(keys);check(number==10&&menu.Changed()==0);
    keys={};keys.left=true;frame(keys);check(number==5);
    keys={};keys.down=true;frame(keys);check(menu.Selected()==3&&menu.Top()==1);
    keys={};keys.left=true;frame(keys);check(choice==2&&menu.Changed()==4);
    keys={};keys.right=true;frame(keys);check(choice==0);
    keys={};keys.down=true;frame(keys);keys={};keys.accept=true;frame(keys);check(menu.Page()==1&&menu.PageChanged());
    frame();check(menu.Selected()==0);
    keys={};keys.accept=true;frame(keys);check(menu.Activated()==10);
    keys={};keys.back=true;frame(keys);check(menu.Page()==0&&menu.PageChanged());
    frame();check(menu.Selected()==4&&menu.Top()==2);
    keys={};keys.down=true;frame(keys);keys={};keys.accept=true;frame(keys);check(menu.Activated()==6);
    keys={};keys.down=true;frame(keys);check(menu.Selected()==0&&menu.Top()==0);
    keys={};keys.up=true;frame(keys);check(menu.Selected()==5);
    keys={};keys.back=true;frame(keys);check(!menu.IsOpen());
    menu.Open();menu.Begin("Dynamic");menu.Action(200,"New stable ID");menu.End();check(menu.Selected()==0&&menu.Top()==0);
    menu.Begin("Duplicate",Input{false,false,false,false,true,false});menu.Action(1,"First");menu.Action(1,"Second");menu.End();check(!menu.Valid()&&menu.Activated()==0);
    Menu<1> tiny;tiny.Open();tiny.Begin("Full");tiny.Action(1,"First");tiny.Action(2,"Second");tiny.End();check(!tiny.Valid()&&tiny.Count()==1);
    tiny.Begin("Invalid ID");tiny.Action(0,"Zero");tiny.End();check(!tiny.Valid());
    tiny.Begin("Invalid range");tiny.Integer(1,"Range",number,10,-10);tiny.End();check(!tiny.Valid());
    tiny.Begin("Invalid choice");tiny.Choice(1,"Choice",choice,nullptr,0);tiny.End();check(!tiny.Valid());
    tiny.Begin("Empty");tiny.End();check(tiny.Valid()&&tiny.Count()==0);
    int32_t extreme=2147483647;tiny.Begin("Overflow",Input{false,false,false,true,false,false});tiny.Integer(1,"Large",extreme,-2147483647-1,2147483647,2147483647);tiny.End();check(extreme==2147483647);
    Repeat repeat;check(repeat.Poll(true,0));check(!repeat.Poll(true,349));check(repeat.Poll(true,350));check(!repeat.Poll(true,351));check(repeat.Poll(true,100000));check(!repeat.Poll(true,100001));check(!repeat.Poll(false,100001));check(repeat.Poll(true,100002));
    Repeat wrap;check(wrap.Poll(true,0xffffff80));check(!wrap.Poll(true,0x20));check(wrap.Poll(true,0xe0));
    char text[8]{};Copy(text,"\xe6\xb1\x89\xe6\xb1\x89\xe6\xb1\x89");check(text[0]==char(0xe6)&&text[3]=='.'&&text[6]==0);
    return (uint64_t(passed)<<32)|failed;
}
#ifndef SHVNX_FREESTANDING_TEST
int main(){return int(uint32_t(shvnx_ui_model_tests()));}
#endif
