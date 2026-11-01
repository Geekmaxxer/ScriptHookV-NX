#pragma once
#include "plugin_memory.h"
namespace ScriptHook {
class PluginHeap {
public:
    static void* Allocate(void* owner,size_t bytes,size_t alignment,uint32_t flags);
    static void* Reallocate(void* owner,void* pointer,size_t bytes);
    static bool Free(void* owner,void* pointer);
    static size_t Size(void* owner,const void* pointer);
    static bool Stats(void* owner,ShvNxMemoryStats&);
    static bool Trim(void* owner);
    // Only loader failure before publication/scheduling may invalidate live data.
    static bool Discard(void* owner);
    static size_t MetadataBytes();
    static bool ResourcePinned(void* owner, const void* pointer);
    static bool SetResourcePin(void* owner, const void* pointer, bool pinned);
    static size_t BlockCapacity();
    static size_t ArenaCapacity();
    static size_t LookupCapacity();
};
}
