# Runtime diagnostics

Optional read-only plugin for diagnosing costly script resumes and callbacks.
Requires the runtime completion test build with the timing API.

Build `main.cpp` with `tools/build_plugin.py`, then install the NRO as
`switch/gtav-scripthook/plugins/runtime-diagnostics.nro.manual`.
Load Now, play normally, then hold **L + R + Nintendo X** to save a snapshot.
One snapshot is also saved after startup and when the diagnostics plugin stops.

Read `switch/gtav-scripthook/settings/runtime-diagnostics/performance-report.txt`.
It lists each plugin's call count, total microseconds and worst invocation for
scripts, callbacks, cleanup and initialization. Counters cover the current GTA
process, including Kill/Load cycles. A script resume excludes time suspended in
WAIT. Elapsed time includes native work, I/O and OS preemption; it is not isolated
CPU usage or a measurement of FPS. Custom natives invoked independently by NSCs
and the game's own work are outside these counters.

No automatic periodic log writes occur. Kill Process removes the overlay;
close GTA to remove the resident NRO. No gameplay state is changed.
