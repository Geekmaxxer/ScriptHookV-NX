# Registration-only PC source example

Original example using familiar ScriptHookV registration syntax. Import this
folder in Mod Porter and create a copied project with standard DllMain adaptation
enabled. Build NRO, then install it as a Manual plugin.

The adapter preserves the worker and render callback in explicit NX initialization
and checks required capabilities. The runtime owns stopping and restarting these
registrations, so the generated cleanup retains them for Load Now. Original files
stay unchanged. Unsupported calls or conditions inside DllMain require review.

Expected result: a line near the left side says the worker and render callback
are active. Kill Process removes it; Load Now restores it. No gameplay state is
changed. This example does not use DirectX or pretend an NX frame is a swap chain.
