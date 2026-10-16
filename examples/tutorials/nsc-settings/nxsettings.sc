USING "rage_builtins.sch"
USING "commands_script.sch"
USING "commands_scripthook_nx.sch"

SCRIPT
    INT settings[3]
    INT loaded = 0
    settings[0] = 1 // Application schema; validate before interpreting old saves.
    settings[1] = 0 // Launch counter.
    settings[2] = -123
    loaded = SHVNX_LOAD(settings[0], 3)
    IF loaded <> 3 OR settings[0] <> 1
        settings[0] = 1
        settings[1] = 0
        settings[2] = -123
    ENDIF
    settings[1] = settings[1] + 1
    SHVNX_SAVE(settings[0], 3)
ENDSCRIPT
