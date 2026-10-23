// Shared custom hashes provided by subsdk5. Count is the number of INTs,
// never the byte length. Namespace 0 uses the calling script's name hash.
NATIVE FUNC BOOL SHVNX_SAVE(INT &buffer, INT count, INT nameSpace = 0) = "0x5348564e58535631"
NATIVE FUNC INT SHVNX_LOAD(INT &buffer, INT count, INT nameSpace = 0) = "0x5348564e584c4431"
NATIVE FUNC BOOL SHVNX_AVAILABLE(INT hashLow, INT hashHigh) = "0x5348564e58415631"

// Plugin Manager Natives (ScriptHookV-NX subsdk5 bridge)
NATIVE FUNC INT SHVNX_GET_PLUGIN_COUNT() = "0x5348564e58504331"
NATIVE FUNC STRING SHVNX_GET_PLUGIN_NAME(INT index) = "0x5348564e58504e31"
NATIVE FUNC STRING SHVNX_GET_PLUGIN_ID(INT index) = "0x5348564e58504931"
NATIVE FUNC BOOL SHVNX_GET_PLUGIN_STATUS(INT index, INT &outStatus, INT &outMemKb, INT &outStep) = "0x5348564e58505331"
NATIVE FUNC STRING SHVNX_GET_PLUGIN_STATUS_TEXT(INT index) = "0x5348564e58505431"
NATIVE FUNC BOOL SHVNX_TOGGLE_PLUGIN(INT index) = "0x5348564e58505447"
NATIVE FUNC BOOL SHVNX_RELOAD_PLUGINS() = "0x5348564e5850524c"

// Manager v3: startup policy 0=Disabled, 1=Manual, 2=Auto.
// Requests are queued. Loaded native code remains resident until GTA closes.
NATIVE FUNC INT SHVNX_PLUGIN_MANAGER_VERSION() = "0x5348564e58505631"
NATIVE FUNC BOOL SHVNX_LOAD_PLUGIN(INT index) = "0x5348564e58504c31"
NATIVE FUNC INT SHVNX_GET_PLUGIN_POLICY(INT index) = "0x5348564e58505031"
NATIVE FUNC BOOL SHVNX_SET_PLUGIN_POLICY(INT index, INT policy) = "0x5348564e58505032"
NATIVE FUNC INT SHVNX_GET_PLUGIN_ACTION_ERROR() = "0x5348564e58504531"
// Cancels cooperative script execution; executable/native mappings remain resident.
NATIVE FUNC BOOL SHVNX_KILL_PLUGIN(INT index) = "0x5348564e58504b31"
NATIVE FUNC BOOL SHVNX_IS_PLUGIN_RUNNING(INT index) = "0x5348564e58505231"
// Optional timing API. kind: 0=script, 1=callback, 2=cleanup, 3=initialize.
// INT arithmetic uses the low 32 bits; C++ GetTiming preserves 64-bit totals.
NATIVE FUNC BOOL SHVNX_GET_PLUGIN_TIMING(INT index, INT kind, INT &calls, INT &totalUs, INT &worstUs) = "0x5348564e58504d31"

