# SDK extensions ABI 1

SDK Parity Test 1 adds these interfaces without changing the original 136-byte
`ShvNxApi` or 40-byte plugin descriptor. Existing ABI 1 NROs remain supported.
New shims expose an optional `shvPluginBindExtensions` export; the runtime binds
it before initialization. Query `getScriptHookCapabilities()` before using an
optional feature. Older runtimes return no extension capabilities through the
new shim. The original caller, scheduler and plugin ABI numbers remain 1.

The implementation has local ARM64 regression coverage and [matched Switch
acceptance](../evidence/sdk-parity-test1-20261022/hardware-20261022/console-acceptance.json).
The combined probe passed smooth PNG rotation/colour, controller input, additional
script execution, Kill/Load, NSC/NRO coexistence and game restart checks. Reports
confirm ped/vehicle/object enumeration and player/global pointer availability.
Dynamic pool counts are confirmed during gameplay. Controller input is the
release requirement; USB keyboard support is optional. Separate alpha/expiry and
instrumented performance measurements remain outside that console acceptance.

## Multiple scripts

`scriptRegisterAdditionalThread(Worker)` registers another `void()` cooperative
entry in the current NRO and returns success. `scriptRegisterChecked` does the
same; `scriptRegister` is the void compatibility form. Each entry has a separate
64 KiB stack and uses `WAIT`. All plugins share the configured script-slot limit
(16 by default); additional entries do not increase that limit. Duplicate active
registration is idempotent. Registering an entry that finished starts it again.

Initialization can stage entries before it returns. Failed initialization rolls
back staged entries. Registrations made during dispatch start on a later frame.
Callbacks must point into the owning NRO's executable text. OS-thread creation
and registering code owned by another plugin are unsupported.

`scriptUnregister(Worker)` / `scriptUnregisterChecked(Worker)` cancel that entry.
`scriptUnregisterAll()` queues a whole-plugin stop. The C++ HMODULE overloads
`scriptRegister(module, Worker)`, `scriptRegisterAdditionalThread(module, Worker)`
and `scriptUnregister(module)` use the owning NRO; the module value is ignored.
Kill Process stops the main entry, all additional entries, callbacks and queued
texture draws, then invokes the optional stop callback once. Load Now restarts
retained entries and callbacks. Returning from the main entry alone does not
stop running children or registered callbacks. Cancellation does not unwind
C++ destructors. Close GTA to replace code or reclaim resident mappings.

## Full world enumeration

```cpp
int handles[128];
int count = worldGetAllVehicles(handles, 128);
for (int i=0; i<count; ++i) {
    // Revalidate before using a handle; process only bounded work per tick.
}
```

`worldGetAllPeds`, `worldGetAllVehicles`, `worldGetAllObjects` and
`worldGetAllPickups` inspect the target's complete corresponding entity pool.
They write ordinary four-byte script handles, up to the supplied capacity, and
return the number written. Null/read-only buffers, invalid contexts or capacities
outside 1..65536 return zero. Order follows pool slots, not distance. Results
are snapshots; deletion or streaming can invalidate them after the call.

Entities without GUIDs receive them using the guarded stock allocator, only
while its freelist has room. Existing handles remain enumerable with a full
GUID pool. Entries that cannot safely receive a handle are skipped. Use modest
capacities and scan infrequently to avoid consuming the game's GUID pool.

Pickup enumeration returns pickup **entity GUIDs**, not placement IDs accepted
by `GET_PICKUP_COORDS`/`DOES_PICKUP_EXIST`. Use entity operations on these handles.
The nearby-ped scanner in `shv/world.h` remains useful for cheaper local queries.

## Globals and entity pointers

`getGlobalPtr(index)` returns a writable eight-byte script-global slot, or
null. It validates the target's 64 bank pointers, actual allocated bank sizes
and full writable allocation; unloaded or out-of-range globals return null.
The index is a 24-bit script-global index, not a byte offset. No PC-to-Switch
global-index translation is performed. Do not alter a global without verifying
its exact purpose and lifetime on this target.

`getScriptHandleBaseAddress(handle)` returns `BYTE*` to the live entity or null.
It checks the GUID pool, slot allocation and generation byte. It rejects stale
handles. Returned pointers are borrowed: never free them, retain them across
`WAIT`, or assume PC entity layouts. Reacquire and revalidate after waits,
streaming, deletion, character changes and loading transitions. Neither API
makes arbitrary pointer dereferences or writes safe.

## Rendering and PNG textures

`renderCallbackRegister(fn)` registers `void(const ShvNxRenderFrame*)`; the frame
contains size, game frame number, viewport width/height and Horizon ticks.
`presentCallbackRegister(fn)` provides the familiar `void(void*)` spelling, with
the same NX frame pointer. This is **not a DirectX swap chain**, a Windows render
thread hook or a graphics-device API. Both run in the game script frame after
cooperative entries and may use verified GTA HUD natives or `drawTexture`.
The callback/frame pointer is valid only during the call. Never call `WAIT`.

```cpp
int id=createTexture("badge.png"); // zero on failure
// Call from a running script or render callback. Rotation is in turns.
drawTexture(id,0,0,0, .10f,.10f, .5f,.5f, .8f,.2f, 0.f,1.f, 1.f,1.f,1.f,1.f);
```

Assets live at `switch/gtav-scripthook/assets/<plugin-id>/<filename>`.
Only flat ASCII filenames up to 63 characters are accepted. Use PNG images up
to 1024x1024 and 4 MiB encoded size. PNG CRCs and dimensions are checked;
decoding uses stb_image with bounded allocation. Images are converted to the
target SGA pixel format. Console testing confirmed colours and smooth rotation;
alpha blending has local coverage but still needs a separate console check.
Load once outside callbacks. A missing file returns zero without drawing.

`drawTexture` accepts texture ID, instance index (0..63), layer, duration in
milliseconds, normalized width/height, anchor, position, rotation in turns,
height multiplier and RGBA multipliers (0..1). Zero duration means one frame.
Positive duration persists until expiry or stop. Reusing an ID/index updates
that instance; layers sort ascending. `drawTextureChecked` accepts the equivalent
`ShvNxTextureDraw` structure and returns whether it was queued.

There are at most 16 cached textures per plugin, 64 overall and 128 simultaneous
draw instances. Decode buffers and conservatively charged GPU resources share
the configured plugin budget (8 MiB by default), so large textures can fail
before dimension limits. The charge is not a measured total GPU footprint.
Cached textures stay resident until GTA exits; repeated creates and restarts
reuse them. Kill removes queued draws, not cached GPU resources. To replace an
image file during testing, restart GTA. Rendering stops submitting sprites when
the target's HUD rectangle buffer is full.

## Controller and keyboard callbacks

`controlHandlerRegister(fn, group, control)` subscribes a
`void(const ShvNxInputEvent*)` callback to a verified GTA PAD action (groups 0..2,
controls 0..359). Events include Pressed, Released, Repeat and Analog, current
normalized value and frame. This observes enabled or disabled GTA actions;
it does not itself suppress gameplay controls. Hold-repeat begins at 500 ms,
then repeats every 100 ms while the game host advances.

`keyboardHandlerRegister(fn)` supports a USB keyboard through the target SDK.
The familiar seven arguments are virtual key, repeat count, scan code,
extended flag, Alt state, previous-down state and key-up flag. Letters, digits,
function/navigation/numpad keys and modifiers use US virtual-key translation;
scan code is a USB HID usage. It is not Windows WM_KEY input, an IME, text input,
or a layout-aware character service. Provide controller bindings for handheld
use. Capability availability verifies SDK code, not that a keyboard is connected. It is optional compatibility support, not a Switch
release test requirement.

The matching `*Unregister` calls remove registrations. There are 16 callback
bindings per plugin, 128 overall, shared across callback types. Additions made
during a callback start later; removal and stop disable future calls. Callback
failures caused by attempted waits or incomplete raw native calls are diagnosed
in the loader report. The completion runtime disables the offending registration
after its first failure; other callbacks continue. Load Now or explicit
re-registration can retry it. Arbitrary crashes and infinite callbacks cannot be isolated
or preempted; keep every callback short and use an additional script for work
that must yield.

## Buildable combined example

```powershell
python tools/create_mod.py --template sdk-parity --id sdk-parity-probe --output projects/parity
python tools/build_mod.py --project projects/parity/project.json --output out/parity-test1 --devkitpro C:/devkitPro
python tools/create_texture_demo.py --output out/palette.png
```

Copy the NRO into `plugins/sdk-parity-probe.nro.manual` and the PNG into
`assets/sdk-parity-probe/palette.png` under `switch/gtav-scripthook/`.
Load Now in Story Mode. The overlay shows pool counts, worker progress, pointer
availability and resolution; Nintendo Y toggles it. The example does not register or poll USB keyboard input. It changes no globals or
entity memory. `settings/sdk-parity-probe/status.txt` records state every five
samples and on stop. Kill/Load should remove and restore its overlay/worker,
without increasing cached texture allocations. Pickup counts may legitimately
be zero and every pool count is capped at 128 in this example.
