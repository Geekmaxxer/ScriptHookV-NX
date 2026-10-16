#pragma once
#include <stdint.h>
#include <stddef.h>
#include "main.h"

namespace ScriptHook {

class Persistence {
public:
    static bool Initialize();
    static bool Save(const char* relativePath, const void* data, size_t size);
    static bool Read(const char* relativePath, void* buffer, size_t maxBytes, size_t* bytesRead);

    // SanScript Custom Native Callbacks
    static void Native_SaveMenuSettings(void* ctx);
    static void Native_LoadMenuSettings(void* ctx);
};

} // namespace ScriptHook
