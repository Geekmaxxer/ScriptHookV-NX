# Supported scope and remaining work

The accepted runtime targets title `0100b00b51230000`, executable build
`6e3e9b3af5746e74b615bd981080c9271bc22662`. Caller, runtime and plugin ABI are
version 1; manager controls are version 3. A game update requires renewed target
verification, not speculative offset changes.

| Area | Current support |
| --- | --- |
| Plugins | Freestanding C++17, AArch64 NROs built for this SDK |
| Familiar syntax | ScriptMain, WAIT, typed native namespaces and raw invocation |
| NSC integration | Generic journaled INT persistence and declared custom NRO natives |
| Discovery | Up to 16 catalog entries; Auto, Manual and Disabled startup policies |
| Resident modules | Eight by default; startup override up to 16 in the standard build |
| Memory limits | Image and BSS each at most 2 MiB; combined loader accounting defaults to 8 MiB (configurable) |
| Scheduling | Sixteen 64 KiB stacks by default; startup override up to 32 |
| CPU budget | Stops starting additional scripts after 2 ms; cannot preempt a runaway plugin |
| Process control | Kill stops all owned scripts/callbacks/draws; Load Now restarts retained registrations |
| SDK extensions | Four entity pools, owned additional scripts, globals/entity pointers, PNG textures and controller/USB keyboard/HUD callbacks; combined probe and changing pool counts passed scoped Switch gameplay acceptance; controller input is required, USB keyboard is optional |
| Replacement | Close GTA to replace mapped code or release a resident slot |

The loader scans at most 4,096 directory entries; scans and initialization are
synchronous work. The scheduler budget does not turn those operations or native
handlers into preemptible jobs. Keep initialization short and save outside frame
loops. Accounted loader memory is not total process RAM or a measured FPS result.

Windows .asi/.dll binaries, Win32/DirectX, arbitrary hosted C++ libraries,
automatic PC hash translation, ScriptHookVDotNet and direct NSC-from-SD loading
are not implemented. Port compatible source and audit native/data contracts.
Unloading code with live cached native pointers remains intentionally unavailable.

See [runtime limits](RUNTIME-LIMITS.md) for player configuration, NRO project
settings and runtime build defaults. All ABIs remain unchanged.

Capacity Test 1 keeps the plugin budget at 8 MiB and all ABIs unchanged. Published
four-resident runtimes retain their previous limits; the current lifecycle runtime has console
acceptance. Full eight-resident stress and custom-limit configurations still
require their own tests. More plugins may reach the memory budget before eight
images load, depending on their image and BSS sizes.

Console evidence covers the caller/runtime samples, external loading, simultaneous
showcase mods, manager use and restored startup modes after restart. New SDK
tutorials have separate local test evidence; they do not inherit console acceptance
just by using the same core. Actual peak RAM/FPS, extended gameplay soak tests,
arbitrary third-party plugin compatibility and future game builds remain work.

The most useful next runtime milestone is measured console performance/soak
coverage with independent third-party ports. Expand platform APIs only for a
concrete port with verified target behavior and a versioned contract. The current
accepted runtime includes optional bounded stop callbacks. Desktop archive
preparation and new assets have separate validation requirements.

The completion test adds per-plugin elapsed timing and isolation of invalid-yield
callbacks. These changes have their own test scope. Eight resident NROs with
sixteen cooperative entries and eight render callbacks are exercised locally;
this is not a console saturation or physical peak-memory measurement.
