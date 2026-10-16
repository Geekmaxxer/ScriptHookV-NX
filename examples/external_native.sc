USING "rage_builtins.sch"
USING "commands_script.sch"
NATIVE FUNC INT SHVNX_EXAMPLE() = "0x5348564558414d31"
NATIVE FUNC BOOL SHVNX_SAVE(INT &buffer, INT count, INT storageNamespace = 0) = "0x5348564e58535631"

SCRIPT
    INT settings[4]
    settings[0] = SHVNX_EXAMPLE()
    settings[1] = 1
    settings[2] = 2
    settings[3] = 3
    SHVNX_SAVE(settings[0], 4)
ENDSCRIPT
