#pragma once
#include "../plugin_memory.h"
#include <new>
#include <type_traits>
#include <utility>
namespace ScriptHook::Memory {
// Explicit cxx_runtime profile. Construction/destruction must not yield.
template<class T,class... Args>T* TryNew(Args&&... args) noexcept{
    static_assert(std::is_nothrow_constructible_v<T,Args...>,"TryNew requires noexcept construction");
    static_assert(std::is_nothrow_destructible_v<T>,"TryNew requires noexcept destruction");
    static_assert(alignof(T)<=4096,"Plugin heap alignment limit is 4096");
    return ::new(std::nothrow)T(std::forward<Args>(args)...);
}
// Complete allocated type, exact owned allocation; no arrays or borrowed pointers.
// Refuses foreign storage before invoking its destructor. Success clears pointer.
template<class T>bool Delete(T*& value) noexcept{
    static_assert(std::is_nothrow_destructible_v<T>,"Delete requires noexcept destruction");
    if(!value)return true;
    if(scriptAllocationSize(value)!=sizeof(T))return false;
    value->~T();const bool released=scriptFree(value);if(released)value=nullptr;return released;
}
}
