USING "rage_builtins.sch"
USING "commands_script.sch"
USING "commands_scripthook_nx.sch"

// The matched .snb declares this dependency before the script can run.
NATIVE FUNC INT NX_ADD(INT a, INT b) = "0x5348564e58414431"

SCRIPT
    INT result[2]
    result[0] = NX_ADD(7, -2) // Expected: 5, from tutorial-provider.nro.
    result[1] = NX_ADD(-10, 3) // Expected: -7, including VM sign extension.
    SHVNX_SAVE(result[0], 2)
ENDSCRIPT
