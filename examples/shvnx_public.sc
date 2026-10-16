USING "rage_builtins.sch"
USING "commands_script.sch"

NATIVE FUNC BOOL SHVNX_SAVE(INT &buffer, INT count, INT storageNamespace = 0) = "0x5348564e58535631"
NATIVE FUNC INT SHVNX_LOAD(INT &buffer, INT count, INT storageNamespace = 0) = "0x5348564e584c4431"

SCRIPT
    INT settings[4]
    settings[0] = 1
    settings[1] = 123
    settings[2] = -456
    settings[3] = 789
    IF SHVNX_SAVE(settings[0], 4)
        SHVNX_LOAD(settings[0], 4)
    ENDIF
ENDSCRIPT
