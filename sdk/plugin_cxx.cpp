// Explicit nothrow subset. Throwing allocation and exception support are absent.
#include "plugin_memory.h"
#include <new>
namespace std {const nothrow_t nothrow{};}
namespace {
void* Allocate(size_t bytes,size_t alignment){return scriptAllocate(bytes?bytes:1,alignment,0);}
void Release(void* pointer){if(pointer)(void)scriptFree(pointer);}
}
void* operator new(size_t n,const std::nothrow_t&) noexcept{return Allocate(n,16);}
void* operator new[](size_t n,const std::nothrow_t&) noexcept{return Allocate(n,16);}
void* operator new(size_t n,std::align_val_t a,const std::nothrow_t&) noexcept{return Allocate(n,size_t(a));}
void* operator new[](size_t n,std::align_val_t a,const std::nothrow_t&) noexcept{return Allocate(n,size_t(a));}
void operator delete(void* p) noexcept{Release(p);}
void operator delete[](void* p) noexcept{Release(p);}
void operator delete(void* p,size_t) noexcept{Release(p);}
void operator delete[](void* p,size_t) noexcept{Release(p);}
void operator delete(void* p,std::align_val_t) noexcept{Release(p);}
void operator delete[](void* p,std::align_val_t) noexcept{Release(p);}
void operator delete(void* p,size_t,std::align_val_t) noexcept{Release(p);}
void operator delete[](void* p,size_t,std::align_val_t) noexcept{Release(p);}
void operator delete(void* p,const std::nothrow_t&) noexcept{Release(p);}
void operator delete[](void* p,const std::nothrow_t&) noexcept{Release(p);}
void operator delete(void* p,std::align_val_t,const std::nothrow_t&) noexcept{Release(p);}
void operator delete[](void* p,std::align_val_t,const std::nothrow_t&) noexcept{Release(p);}
