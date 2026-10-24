# PC gameplay port expedition

The [PC gameplay ports](../mods/pc-ports-test1/README.md) progress from a speed display
and small ambient/QoL features to persistent inventory and multi-stage world
missions. They use native C++ adaptations of pinned MIT sources. They do not
implement a managed C# runtime or automatic ASI translation.

Original gameplay data is retained where used: the mine vehicle/weapon sets,
four pizza shops, 50 jammer locations, treasure note/clue/chest tables and the
slasher investigation sites. Windows input, NativeUI/SHVDN UI, XML storage and
PC phone/camera presentation are replaced by bounded Switch implementations.
Read each `PORT.md` before treating a PC feature as supported.

The local test scope executes the compiled ARM64 plugins on the accepted
production runtime with mocked game, filesystem, input and OS services. It
checks complete treasure/slasher stage transitions, inventory journaling,
rewards, model failures and lifecycle cleanup. This is not Switch gameplay,
visual-quality or FPS verification. The gameplay test ZIP contains no runtime,
NSC menu, settings defaults or game archives.

Test 2 replaces four-button chords with three-button shortcuts, adds visible
status/objective badges and improves vehicle, wallet and mission feedback.
Its installer updates existing Auto/Manual/Disabled filenames in place. These
refinements have separate local tests and require Switch gameplay acceptance.
See [Test 2 changes](../mods/pc-ports-test1/CHANGES-Test2.md) and
[controls](../mods/pc-ports-test1/CONTROLS.md). The pure shortcut state test is
`mods/pc-ports-test1/tests/shortcut_tests.cpp`; build it with a host C++17 compiler.

## Larger candidates

[MenyooSP](https://github.com/MAFINS/MenyooSP/tree/b894e14f31490fbb91740dc554111bda5be39608)
has 218 C/C++ source/header files in the audited revision. Its memory layer uses
PC pattern scans, `VirtualProtect` and module-address queries. Those paths need
new Switch implementations; an ARM64 build alone cannot make them work.
Its GPL license also stays separate from the ten MIT upstreams.

[SimpleMenu](https://github.com/Mouataz86/SimpleMenu/tree/3b43f609f889955c2327f7b10622baa2a1eaff4b)
is a smaller trainer candidate, but its C#/SHVDN/LemonUI implementation requires
a deliberate C++ and controller-UI rewrite. No trainer NRO is included in Test 1.

Laser Sights and Persistent Weapon Flashlight were also inspected. Their PC
implementations modify rendering/weapon-memory paths; a generic line or light
effect is not a complete source port. TornadoScript likewise needs a separate
audit of its memory layer and particle dependencies before its overhaul can be
claimed as supported. These candidates remain research, not playable downloads.

Further SDK changes should follow concrete port requirements and retain the
existing ABI and accepted runtime until matched console tests establish a change.
