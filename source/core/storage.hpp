#pragma once
#include "memory.hpp"
#include "bindings.hpp"
namespace ScriptHook {
template<class F> F sdk_call(uint32_t at) { return reinterpret_cast<F>(Memory::s_sdkBase + at); }
template<class F> F main_call(uint32_t at) { return reinterpret_cast<F>(Memory::s_mainBase + at); }
// The target SDK writes nn::fs::DirectoryEntry with a one-byte entry type,
// three padding bytes and an eight-byte size. A 0x318 stride corrupts every
// name after the first item in a directory read.
struct DirectoryEntry { char name[0x301]; uint8_t padding[3]; uint8_t type, reserved[3]; int64_t bytes; };
static_assert(offsetof(DirectoryEntry, type) == 0x304);
static_assert(offsetof(DirectoryEntry, bytes) == 0x308);
static_assert(sizeof(DirectoryEntry) == 0x310);
class Storage {
    uintptr_t previous = 0;
    bool scoped = false;
public:
    bool mounted = false;
    uint32_t error = 0;
    bool rename_attempted = false;
    uint32_t rename_result = UINT32_MAX, rename_postcheck = 0;
    Storage();
    ~Storage();
    bool Read(const char* relative, void* out, size_t capacity, size_t& bytes);
    bool Write(const char* relative, const void* data, size_t bytes);
    bool Size(const char* relative, size_t& bytes);
    bool Rename(const char* fromRelative, const char* toRelative);
    bool Delete(const char* relative);
    bool Plugins(DirectoryEntry* entries, size_t capacity, size_t& count);
    bool Bindings(DirectoryEntry* entries, size_t capacity, size_t& count);
    bool List(const char* relative,DirectoryEntry* entries,size_t capacity,size_t& count);
    // Read successive pages using one directory handle. The startup guard is
    // reported explicitly instead of silently truncating a large directory.
    bool Scan(const char* relative,DirectoryEntry* entries,size_t capacity,
              void (*visit)(const DirectoryEntry&,void*),void* context,size_t& count);
};
}
