# Audited targets

Each profile identifies a title/build and hashes its `bindings.hpp` and
`bindings.json`. Profiles contain small compatibility constants and fingerprint
guards derived from the verified executable/SDK. They contain no complete game
image, game asset or proprietary compiler.

Normal builds copy the selected verified profile and need no game extraction.
The runtime still checks size and fingerprint contracts before hooking. A game
update is not automatically compatible; re-audit the target before changing
offsets or guards. `runtime-reference.json` records the initialized segment hashes
of the accepted runtime and allows only its valid GNU build ID to differ during
an unmodified source rebuild. See the [core build guide](../docs/DEVELOPING.md).
