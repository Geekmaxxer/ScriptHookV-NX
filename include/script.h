#pragma once
#include "plugin.h"
#include "natives.h"

// Familiar source syntax, using this Switch build's native declarations.
// This does not provide Windows DLL/ASI or PC native-hash compatibility.
using namespace ScriptHook::Natives;

// WAIT(0) resumes on the next game frame. Longer waits use milliseconds.
// Call only from ScriptMain (or its callees), never from plugin initialization.
inline void WAIT(uint32_t milliseconds) { scriptWait(milliseconds); }

namespace ScriptHook { namespace PluginDetail {
template <size_t N> constexpr bool ValidId(const char (&id)[N]) {
    if (N < 2 || N > 32 || id[N - 1]) return false;
    for (size_t i = 0; i + 1 < N; ++i) {
        const char c = id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
    }
    return true;
}
} }

// Use once per plugin, outside any namespace, after the entry function.
// The plugin ID also namespaces settings files; keep it stable between builds.
#define SHV_NX_PLUGIN(plugin_id, entry_function) \
    static_assert(::ScriptHook::PluginDetail::ValidId(plugin_id), \
                  "Plugin ID requires 1-31 ASCII letters, digits, underscores or hyphens"); \
    extern "C" SHV_NX_API const ShvNxPlugin* shvPluginQuery() { \
        static constexpr ShvNxPlugin descriptor = { \
            sizeof(ShvNxPlugin), SHV_NX_PLUGIN_ABI_VERSION, \
            SHV_NX_CALLER_ABI_VERSION, SHV_NX_RUNTIME_ABI_VERSION, \
            plugin_id, shvBindApi, entry_function }; \
        return &descriptor; \
    }
