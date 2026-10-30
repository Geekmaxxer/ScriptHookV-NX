# Optional helpers

For controller menus, pages, selectors and measured text layout, use the
[reusable UI framework](CONTROLLER-UI.md).

These headers compile into the plugin. They add no core ABI, background thread,
heap allocation or automatic disk polling. Include only what your mod needs.

## Timers: `shv/time.h`

`Util::Every::Ready(now, interval)` returns true on its first call, then once per
interval. It samples the current time and skips missed periods instead of running
an expensive catch-up loop. Pass `uint32_t(MISC::GET_GAME_TIMER())` and intervals
below 2^31 milliseconds. `Due` and `Elapsed` handle the counter wrapping. Sampling
must occur within that half-range. Your loop still needs `WAIT`.

## Controller input and INI files

`<shv/input.h>` supplies Nintendo-labeled frontend actions and a once-per-press
`Input::ShoulderChord` for L + R + Y. Poll once per frame. Supply `Button::X` to
select X instead. Controls use game action mappings, not raw controller hardware.

`<shv/ini.h>` supplies a fixed-size `Util::Ini<>` reader:

```cpp
ScriptHook::Util::Ini<> config;
(void)config.Load("config.ini");
bool enabled = config.Bool("Effects", "Enabled", false);
int interval = config.Int("Effects", "IntervalMs", 1000, 100, 10000);
```

Load during initialization. The filename is namespaced by plugin ID, and missing
or invalid files leave the supplied defaults in effect. UTF-8, sections, whole-line
comments and duplicate keys are supported. Quoting and automatic saves are not.
See [API contracts](API-REFERENCE.md) for capacities and optional stop cleanup.

## Settings: `shv/settings.h`

```cpp
ScriptHook::Util::SavedSettings<2> settings(0x4d4f4431u, 1);
uint32_t words[2] = {1, 50};
(void)settings.Load(words);
// Validate ranges here before using saved values.
words[1] = 75;
bool saved = settings.Save(words);
```

The tag identifies your record format; the second argument is its schema.
One to 256 words are supported. Records contain a tag, schema, generation, word
count and CRC. Load chooses the newest complete valid slot and leaves defaults
intact if neither is usable. Save writes the other slot and checks a full readback
before updating its in-memory generation. Failure returns false. This reduces
damage from partial writes; it is not a power-loss/SD durability guarantee.

Default filenames are `settings-a.bin` and `settings-b.bin`, isolated by the plugin
ID. Supply different flat filenames in constructor arguments three/four for
multiple records in one plugin. Keep the object in ScriptMain/local state;
dynamic global constructors are unsupported. Save changes deliberately, not each
frame. No destructor autosave exists: Kill Process discards suspended stacks.

## Text: `shv/text.h`

`TextBuffer<N>` holds bounded UTF-8 plain text and numbers without printf or a
hosted standard library. `Add`, `UInt`, `Int`, `CStr`, `Size` and `Complete` expose
composition and truncation. It never emits a partial UTF-8 scalar; invalid text
or insufficient space sets Complete false. Use it for short HUD text or reports.

`AppendHudText(text)` belongs between matching HUD BEGIN/END text commands. It
splits valid plain UTF-8 into at most ten components of at most 99 bytes each,
matching the target HUD's component limit. It returns false for invalid text or
overflow; a valid prefix may already have been submitted. It does not measure or
position text and does not understand GTA rich-text tokens. Use it for plain
strings and set wrapping/style yourself, as the overlay tutorial demonstrates.

All game/native/file operations still belong to the selected script thread.
Do not call these I/O or HUD helpers from custom native callbacks, OS threads,
query callbacks or static initialization.
