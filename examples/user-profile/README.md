# Active user profile example

This example reads the nickname of the single open Switch user through the
optional profile service and writes it to its own `profile-name.txt` settings
file. It retries initialization/no-user states once per second for up to 30
attempts. It does not select, open, close or authenticate an account.

```powershell
python tools/build_mod.py --project examples/user-profile/project.json --output out/user-profile --devkitpro C:/devkitPro
```

The example requires the unreleased profile service. Close GTA before replacing
code. Verify `settings/user-profile-example/profile-name.txt` against the profile
selected for the game; remove or protect that file before sharing a settings dump.
Ambiguous selections and invalid names are not replaced with fabricated values.

See the [profile API](../../docs/reference/USER-PROFILE.md) for exact status,
UTF-8 and emulator behavior. The [initial physical run](../../docs/verification/PLATFORM-CAPACITY1.md#physical-test1-demo-run)
matched one selected Switch profile. A second profile and the intended Eden
launch/account workflow remain separate acceptance checks.
