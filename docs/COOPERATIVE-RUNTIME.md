# Cooperative runtime ABI 1

`scriptWait(ms)` suspends the current C++ stack and resumes after the delay on
a later game frame. `scriptWait(0)` resumes on the next observed frame. Returning
from a script ends it; its entry function is not called again automatically.

```cpp
#include "natives.h"

void MainScript() {
    unsigned samples = 0;
    for (;;) {
        const auto timer = ScriptHook::Natives::MISC::GET_GAME_TIMER();
        if (getNativeCallStatus() != NativeCall_Ok) return;
        ++samples; // This local survives the wait.
        (void)timer;
        scriptWait(1000);
    }
}
// A built-in module can register MainScript through scriptRegisterChecked.
// An external plugin exports this through SHV_NX_PLUGIN or its descriptor.
```

The console-accepted Test 1 driver used the matched MEGATARD menu's VM tick, after the
original interpreter returns. It requires a live thread ID, the paired program's
identity/cache checks, an idle native caller and an inactive outer script context.
It temporarily installs the live thread's script TLS/global context and auxiliary
state, then restores their exact previous values. This borrows the host's script
identity; it does not create an independently owned RAGE script thread. Natives
that terminate, wait on or otherwise alter that host are unsuitable here.

Dispatch uses the real target `GET_FRAME_COUNT` handler, so multiple VM callbacks
in one frame cannot run the same script twice. Wait deadlines use Horizon system
ticks. Pause time counts toward a timed wait; scripts progress only when their
host ticks and the frame count changes. New registrations during a dispatch wait
until the next frame. A returned function releases its slot; re-registering it
starts with a fresh stack. Duplicate registration of an active function is harmless.

The default pool has sixteen fixed 64 KiB stacks (1 MiB). Optional startup
configuration can select up to 32 slots in the standard build; only additional
stacks allocate pages, once before plugins start. There are no sleeping OS
threads or scripts registered at module startup. See [runtime limits](RUNTIME-LIMITS.md).
Each frame permits at most one resume per configured slot. A two-millisecond elapsed-time check
after each resume defers further work fairly; it cannot interrupt a script that
never yields. Plugins must call `scriptWait` regularly. Stack canaries detect
boundary damage at yields/dispatch; they are not guard pages or fault recovery.

The earlier console-accepted runtime has eight stacks. The sixteen-stack build
keeps ABI 1 and the same two-millisecond budget; its increased capacity requires
separate Switch acceptance.

The ARM64 switch preserves x19-x30, SP, d8-d15 and FPCR/FPSR with 16-byte stack
alignment. It leaves x18 and the OS thread pointer untouched. The register and
stack rules follow [Arm's AAPCS64](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst).
Each suspended script also retains its last native status/hash. Yielding inside
a native handler or with unfinished raw native arguments is rejected; finish the
native call before waiting. An unfinished call at script return is discarded.

After the first dispatch, script control belongs to that OS thread; worker-thread
registration, cancellation and waits fail. Before dispatch, initialization must
register scripts from a single thread. The original Test 1 cancelled stacks when its menu host changed. The public
loader instead accepts any valid live host, retaining independent stacks across
host changes and skipping killed hosts. `scriptUnregister` discards a suspended stack;
self-unregistration and `scriptStopAll` take effect by the next yield/return for
the running script. Cancellation does not unwind C++ destructors. Release owned
game resources before unregistering; module unloading is not supported yet.

`scriptRegisterChecked`, `getScriptRuntimeStatus`, `getScriptRuntimeInfo` and
`getScriptRuntimeAbiVersion` expose registration results and bounded diagnostics.
`scriptRegister`, `scriptUnregister` and `scriptWait` retain their existing C
signatures. This is a source-port API, not PC ASI/DLL binary compatibility.

## Console test

Replace only subsdk5 using the EASY test ZIP. With the current MEGATARD menu,
run **Persistent Settings > Storage Self-Test**. This explicitly schedules two
read-only C++ scripts that check stack locals, next-frame waits, a 100 ms delay,
fair progress, real native invocation and normal completion. They write
`switch/gtav-megatard/runtime-report.txt` once when done. Expect eight passes,
zero failures, two completions, zero active scripts and zero stack failures.
`bridge-report.txt` retains the seven-getter and storage diagnostics. No runtime
test runs during Story Mode startup. Local emulation does not establish console
acceptance; test fresh boot, menu access, the explicit test and saved settings.

## Public build checks

```powershell
python tools/test_portable.py
python tools/build.py --devkitpro C:/devkitPro --output out/my-runtime-test
python tools/check_examples.py --devkitpro C:/devkitPro --output out/my-examples
```

The original maintainer workspace retains additional target-instruction
emulator checks, which need external game research inputs and are separate from
these public checks. That emulator executes the actual scheduler, assembly switch, native
caller and target getter instructions. Separate C++ test scripts forward through
the public API into that same module. OS queries/time/filesystem, outer VM tick
and game state are fixtures. Both adapters are excluded from the install ZIP.

Test 1 console acceptance: eight checks passed, 115 ms elapsed for the 100 ms
wait, five peer steps, two normal completions and zero context/stack/budget
errors. The new [public loader](PUBLIC-SDK.md) retains this runtime ABI and adds
general host selection and external modules. External loading and subsequent
simultaneous-plugin/manager paths have their own console acceptance records linked
from the repository README. New plugins still require their own gameplay tests.
