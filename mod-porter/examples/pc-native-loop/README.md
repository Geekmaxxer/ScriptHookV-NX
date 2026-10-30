# PC entry-point example

Open this folder in Mod Porter, analyze it and create a project with the standard
DllMain adapter enabled. The tool replaces PC includes, maps GAMEPLAY to MISC,
and adds an NX descriptor. Build the copied project with the selected SDK.

The example reads the player handle and timer once per second. It adds no HUD
and changes no game state. Compilation does not establish console acceptance.
