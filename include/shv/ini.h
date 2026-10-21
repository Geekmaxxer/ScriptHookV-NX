#pragma once
#include "plugin.h"
#include "text.h"

namespace ScriptHook { namespace Util {
// Strict bounded UTF-8/ASCII INI reader. No allocation or per-frame file reads.
// Duplicate keys use the last value. Unsupported quoting/invalid lines fail
// the whole parse; callers keep their defaults. Filenames are plugin-namespaced.
template<size_t Bytes=4096,size_t Entries=64> class Ini {
    static_assert(Bytes>=32&&Bytes<=16384&&Entries>=1&&Entries<=128);
    struct Entry { char* section;char* key;char* value; };
    char text[Bytes]{};
    Entry entries[Entries]{};
    size_t count=0;
    static char Lower(char c){return c>='A'&&c<='Z'?char(c+('a'-'A')):c;}
    static bool Same(const char* a,const char* b){if(!a||!b)return false;while(*a&&*b)if(Lower(*a++)!=Lower(*b++))return false;return !*a&&!*b;}
    static char* Trim(char* s){while(*s==' '||*s=='\t')++s;size_t n=0;while(s[n])++n;while(n&&(s[n-1]==' '||s[n-1]=='\t'||s[n-1]=='\r'))s[--n]=0;return s;}
    bool Fail(){count=0;return false;}
public:
    bool Parse(const char* source,size_t bytes){
        count=0;if(!source||bytes>=Bytes)return false;
        for(size_t i=0;i<bytes;++i){if(!source[i])return false;text[i]=source[i];}text[bytes]=0;
        char* line=text;const auto* raw=reinterpret_cast<const uint8_t*>(line);
        if(bytes>=3&&raw[0]==0xef&&raw[1]==0xbb&&raw[2]==0xbf)line+=3;
        for(size_t i=0;i<bytes;){const auto unit=Utf8Unit(text+i);if(!unit)return Fail();i+=unit;}
        char* section=nullptr;
        while(*line){
            char* next=line;while(*next&&*next!='\n')++next;if(*next)*next++=0;
            char* value=Trim(line);line=next;
            if(!*value||*value==';'||*value=='#')continue;
            if(*value=='['){
                char* end=value;while(*end&&*end!=']')++end;
                if(!*end)return Fail();
                *end++=0;end=Trim(end);
                if(*end&&*end!=';'&&*end!='#')return Fail();
                section=Trim(value+1);if(!*section)return Fail();continue;
            }
            char* split=value;while(*split&&*split!='=')++split;if(!*split)return Fail();*split++=0;
            char* key=Trim(value);char* item=Trim(split);
            if(!*key||count==Entries||*item=='\"'||*item=='\'')return Fail();
            entries[count++]={section,key,item};
        }
        return true;
    }
    bool Load(const char* filename="config.ini"){
        count=0;size_t bytes=0;
        if(!scriptReadFile(filename,text,Bytes-1,&bytes)||bytes>=Bytes)return false;
        // Parse supports this buffer as its own source.
        return Parse(text,bytes);
    }
    const char* Get(const char* section,const char* key,const char* fallback="")const{
        if(!section)section="";
        for(size_t i=count;i;--i){const auto& e=entries[i-1];if(Same(e.section?e.section:"",section)&&Same(e.key,key))return e.value;}
        return fallback;
    }
    int32_t Int(const char* section,const char* key,int32_t fallback,int32_t minimum=INT32_MIN,int32_t maximum=INT32_MAX)const{
        const char* s=Get(section,key,nullptr);if(!s||!*s||minimum>maximum)return fallback;
        bool negative=*s=='-';if(negative||*s=='+')++s;if(!*s)return fallback;
        uint64_t number=0;while(*s){if(*s<'0'||*s>'9')return fallback;number=number*10u+uint32_t(*s++-'0');if(number>2147483648ULL)return fallback;}
        const int64_t value=negative?-static_cast<int64_t>(number):static_cast<int64_t>(number);
        return value<minimum||value>maximum?fallback:static_cast<int32_t>(value);
    }
    bool Bool(const char* section,const char* key,bool fallback)const{
        const char* value=Get(section,key,nullptr);if(!value)return fallback;
        if(Same(value,"true")||Same(value,"yes")||Same(value,"on")||Same(value,"1"))return true;
        if(Same(value,"false")||Same(value,"no")||Same(value,"off")||Same(value,"0"))return false;
        return fallback;
    }
};
} }
