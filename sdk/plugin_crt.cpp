// Opt-in freestanding C subset. Allocation belongs to this NRO's memory service.
// No errno, locale, stdio, hosted C++ runtime or global constructors are supplied.
#include "plugin_memory.h"
extern "C" {
void* malloc(size_t n){return scriptAllocate(n,16,0);}
void* calloc(size_t n,size_t width){return scriptCalloc(n,width);}
void* realloc(void* p,size_t n){return scriptReallocate(p,n);}
void free(void* p){(void)scriptFree(p);}
void* aligned_alloc(size_t alignment,size_t n){return !alignment||n%alignment?nullptr:scriptAllocate(n,alignment,0);}
void* memmove(void* dst,const void* src,size_t n){
    auto* out=static_cast<unsigned char*>(dst);auto* in=static_cast<const unsigned char*>(src);
    if(reinterpret_cast<uintptr_t>(out)<reinterpret_cast<uintptr_t>(in)){for(size_t i=0;i<n;++i)out[i]=in[i];}
    else if(out!=in){for(size_t i=n;i;--i)out[i-1]=in[i-1];}
    return dst;
}
int memcmp(const void* a,const void* b,size_t n){auto* x=static_cast<const unsigned char*>(a);auto* y=static_cast<const unsigned char*>(b);for(size_t i=0;i<n;++i)if(x[i]!=y[i])return x[i]<y[i]?-1:1;return 0;}
void* memchr(const void* p,int c,size_t n){auto* text=static_cast<const unsigned char*>(p);for(size_t i=0;i<n;++i)if(text[i]==static_cast<unsigned char>(c))return const_cast<unsigned char*>(text+i);return nullptr;}
size_t strlen(const char* s){size_t n=0;while(s[n])++n;return n;}
size_t strnlen(const char* s,size_t limit){size_t n=0;while(n<limit&&s[n])++n;return n;}
int strcmp(const char* a,const char* b){while(*a&&static_cast<unsigned char>(*a)==static_cast<unsigned char>(*b)){++a;++b;}return static_cast<unsigned char>(*a)-static_cast<unsigned char>(*b);}
int strncmp(const char* a,const char* b,size_t n){for(size_t i=0;i<n;++i){const auto x=static_cast<unsigned char>(a[i]),y=static_cast<unsigned char>(b[i]);if(x!=y)return x-y;if(!x)return 0;}return 0;}
char* strcpy(char* out,const char* in){char* p=out;while((*p++=*in++)){}return out;}
char* strncpy(char* out,const char* in,size_t n){size_t i=0;for(;i<n&&in[i];++i)out[i]=in[i];for(;i<n;++i)out[i]=0;return out;}
char* strcat(char* out,const char* in){strcpy(out+strlen(out),in);return out;}
char* strncat(char* out,const char* in,size_t n){size_t end=strlen(out),i=0;for(;i<n&&in[i];++i)out[end+i]=in[i];out[end+i]=0;return out;}
char* strchr(const char* s,int c){const char wanted=static_cast<char>(c);for(;;++s){if(*s==wanted)return const_cast<char*>(s);if(!*s)return nullptr;}}
char* strrchr(const char* s,int c){const char wanted=static_cast<char>(c);const char* found=nullptr;do{if(*s==wanted)found=s;}while(*s++);return const_cast<char*>(found);}
char* strstr(const char* text,const char* match){if(!*match)return const_cast<char*>(text);for(;*text;++text){size_t n=0;while(match[n]&&text[n]&&match[n]==text[n])++n;if(!match[n])return const_cast<char*>(text);}return nullptr;}
}
