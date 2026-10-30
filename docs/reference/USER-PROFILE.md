# Active user profile

The unreleased profile service reads the nickname of the single open Switch user
from the game's initialized account service. It supplies a local display name for
source ports without requiring a platform login flow or a configured mod username.

## Interface

Include `script.h` or `plugin_profile.h`. Check `scriptProfileAvailable()` and call
`scriptGetUserProfile(&profile)` from the owning plugin thread. The independent
profile ABI 1 table is 16 bytes; `ShvNxUserProfile` is 56 bytes. Existing plugin,
extension and resource tables retain their layouts.

```cpp
ShvNxUserProfile profile{};
if (scriptGetUserProfile(&profile) == ShvNxProfile_Ok) {
    // profile.name is a validated, NUL-terminated UTF-8 display name.
}
```

The nickname is at most 32 bytes plus its terminator. Valid multi-byte UTF-8 is
preserved. Empty, malformed, unterminated and control-bearing names are rejected.
Failure clears the returned name. The structure also includes ABI/size fields,
the operation status and the original platform result when an SDK operation fails.
Invalid pointers, lengths and foreign-thread calls are rejected before platform
access; those rejected core calls do not write the supplied buffer.

| Status | Meaning |
| --- | --- |
| `Ok` | Exactly one valid open user has a valid nickname |
| `Unavailable` | The installed runtime did not bind the optional service |
| `NotInitialized` | The game's account service is not initialized |
| `NoOpenUser` | No valid open user is available |
| `AmbiguousUser` | Several users are open; selection is not inferred |
| `PlatformError` | An SDK request failed or returned an invalid count |
| `InvalidName` | Nickname validation failed |
| `WrongThread` | The caller is outside its owning loader thread |
| `InvalidArgument` | The output pointer or sized core request is invalid |
| `UnsupportedTarget` | SDK resolution or an audited instruction guard failed |

Service availability describes the optional ABI binding; the operation result
describes current account state. Query when needed rather than every frame. A
later query observes nickname/account-state changes. No account is initialized,
selected, opened or closed by this service. Account IDs, authentication tokens and
network credentials are not returned. Nicknames are display values and need not be
unique; they are not an authentication or persistence identity.

The [complete example](../../examples/user-profile/README.md) records the name in
its own settings directory. The runtime does not replace the game's player-name
natives or automatically publish profile data to a server.

## Target verification

IDA Pro 9.5 inspection verified the SDK exports for initialized state, the open-user
list and nickname retrieval. Their first instructions, offsets, sizes and normalized
SDK fingerprint are recorded in `profile-api.json` in the audited target profile.
The runtime checks these guards before invoking the SDK. The nickname wrapper
copies 32 bytes and writes its terminator at byte 32; open-user enumeration uses
16-byte UIDs and a maximum of eight entries. UIDs remain temporary internal data.

This implementation uses the existing account service rather than consuming or
replaying the game's launch parameters. It returns an explicit ambiguity result
when the open-user state cannot identify one profile.

## Emulators and acceptance

On an emulator, the SDK delegates to its emulated account service. The expected
nickname is therefore the profile that service presents as the single open user;
this follows the same interface and has not been accepted in Eden.

An [Eden issue report](https://github.com/eden-emulator/Issue-Reports/issues/706)
documents disagreement between the HOME-menu selector and the configured user in
v0.2.1 on Linux. This report does not establish behavior for other builds. Test
two distinct profile names, direct launch and HOME-menu launch, and verify that
the returned name matches the account actually used by the game and its save.

The [libnx account interface](https://github.com/switchbrew/libnx/blob/master/nx/include/switch/services/acc.h)
provides public context for the platform's profile data. This runtime calls the
independently audited SDK wrappers rather than linking libnx into GTA.

See [verification and hardware acceptance](../verification/PLATFORM-CAPACITY1.md).
Local execution models the account SDK boundary and Horizon replies; physical
Switch and Eden behavior remain separate acceptance requirements.
