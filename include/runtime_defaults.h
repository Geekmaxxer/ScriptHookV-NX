#pragma once
// Runtime-builder defaults. tools/build.py also accepts overrides for these.
// NRO authors/users: generate runtime.ini with tools/configure_runtime.py.
#define SHVNX_DEFAULT_RESIDENT_PLUGINS 8
#define SHVNX_DEFAULT_SCRIPT_SLOTS 16
#define SHVNX_DEFAULT_PLUGIN_MEMORY_MIB 8
// Startup overrides are bounded by these compiled ceilings. The discovery
// catalog has 16 entries; increasing that catalog requires a separate change.
#define SHVNX_MAX_RESIDENT_PLUGINS 16
#define SHVNX_MAX_SCRIPT_SLOTS 32
#define SHVNX_MAX_PLUGIN_MEMORY_MIB 64

#if defined(__cplusplus)
static_assert(SHVNX_DEFAULT_RESIDENT_PLUGINS >= 1 && SHVNX_DEFAULT_RESIDENT_PLUGINS <= SHVNX_MAX_RESIDENT_PLUGINS);
static_assert(SHVNX_DEFAULT_SCRIPT_SLOTS >= 1 && SHVNX_DEFAULT_SCRIPT_SLOTS <= SHVNX_MAX_SCRIPT_SLOTS);
static_assert(SHVNX_DEFAULT_PLUGIN_MEMORY_MIB >= 1 && SHVNX_DEFAULT_PLUGIN_MEMORY_MIB <= SHVNX_MAX_PLUGIN_MEMORY_MIB);
static_assert(SHVNX_MAX_RESIDENT_PLUGINS >= 1 && SHVNX_MAX_RESIDENT_PLUGINS <= 16);
static_assert(SHVNX_MAX_SCRIPT_SLOTS >= 1 && SHVNX_MAX_SCRIPT_SLOTS <= 64);
static_assert(SHVNX_MAX_PLUGIN_MEMORY_MIB >= 1 && SHVNX_MAX_PLUGIN_MEMORY_MIB <= 256);
#endif
