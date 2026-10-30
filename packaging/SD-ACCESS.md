# Shared SD access

GTA's process needs explicit SD permission to load NROs and save mod settings.
The included main.npdm grants it for title 0100b00b51230000 and the supported
game build. It is shared by ScriptHookV-NX, Self Radio and the MEGATARD menu.

Close GTA. For a standard installation, copy atmosphere to the SD root. If your
existing main.npdm has custom changes, run Install-SD-Access.cmd instead: it
backs up that file, adds only SdCard to both permission masks and preserves
everything else. Already-enabled metadata is left intact. Then copy the other
mod/runtime files while preserving that patched main.npdm.

This contains process permission metadata, not GTA executable code or assets.
