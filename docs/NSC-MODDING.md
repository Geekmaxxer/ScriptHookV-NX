# SanScript and NSC mods

SanScript scripts and native plugins use different loaders. An NRO goes in the
SD plugin folder; a compiled NSC remains a game script resource in `script_rel.rpf`.
The plugin loader does not turn SD NSCs into game resources. You can use generic
persistence and custom NRO natives without naming your script `ragemenu`.

## Build a persistence example

Requirements: the separate MG-SwitchMenu SDK, its .NET/PowerShell requirements,
your SanScript compiler tree and a stock NSC/header reference from the exact
Switch build. These game/compiler files are not in this developer ZIP.

```powershell
python tools/create_mod.py --template nsc-settings --id nxsettings --output projects/nxsettings
python tools/build_mod.py --project projects/nxsettings/project.json --output out/nxsettings-test1 --menu-sdk "C:/modding/MG-SwitchMenu SDK" --dev-ng X:/gta5/script/dev_ng --header-reference "C:/modding/stock/script_rel.rpf"
python tools/inspect_mod.py out/nxsettings-test1/nxsettings.snb
```

The builder calls the established MG build engine and header adapter. It emits
`nxsettings.nsc`, the matched `nxsettings.snb`, source snapshots and manifests.
It does not hand-edit header pointers or install anything. NRO projects can list
multiple sources/includes; NSC projects currently use one .sc and flat sibling
.sch files, resolved by the SanScript compiler.

Close GTA. Insert the **raw adapted `.nsc`** into
`update2.rpf/switch/levels/gta5/script/script_rel.rpf` as a **normal compressed
file entry**, then copy its `.snb` to `switch/gtav-scripthook/bindings/`.
Use the menu's Custom NSC Loader or your existing script launcher to start it.
The official default script stack pool size is **1424**; do not guess arbitrary
stack sizes such as 4096/8192. Never install `.pc.nsc`, rename the compiled NSC,
or create an RPF resource entry for it.

SHVNX_SAVE/LOAD counts are **INT words**, up to 256, not byte sizes. Namespace
zero uses the calling script's name hash. A shared explicit namespace is an
intentional API decision; otherwise distinct scripts keep separate settings.
Generic NSC journals live at `settings/nsc-<eight-digit-hash>/a.bin` and `b.bin`.
The updated runtime migrates the previous flat slot filenames on access, retaining
their generation and CRC. MEGASAVE/MEGALOAD storage in `gtav-megatard` is unchanged.

## Connect an NRO provider

Build the `native-provider` NRO tutorial and generate `nsc-provider-client` as
`nxclient`. Build the client with the same command pattern. Its project includes
the demonstration hash in `custom_hashes`; `build_mod.py` passes it to the binding
generator automatically. Install provider, client and matched binding together.
The provider's run entry may finish: its custom native remains mapped and callable.

Rebuilding **any** NSC changes its binding identity. Regenerate `.snb` from that
exact final `.nsc` even when the native declarations seem unchanged. For your
existing build workflow use:

```powershell
python tools/native_bindings.py --script path/to/mymod.nsc --hash 0x5348564e58414431 --output out/mymod.snb
```

SHVNX built-ins are selected automatically; repeat `--hash` for other provider
dependencies. Keep the manifest guard installed while dependent scripts can run.
A missing provider holds declared dependent scripts rather than making their
custom native calls safe without the core. An in-script capability check cannot
protect an unregistered native before script binding; use the matched runtime.

The small .snb contains custom binding identities, not full debugging symbols or
decompiled source. Fifteen entries require 304 bytes. A separate compiler debug
database is a different artifact regardless of a similar extension.
