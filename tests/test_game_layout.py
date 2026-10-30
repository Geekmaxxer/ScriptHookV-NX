"""Exercise player recovery against disposable SD fixtures, never a mounted card."""
import bootstrap
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from sd_permissions import ROOT, stage


@unittest.skipUnless(shutil.which('powershell') or shutil.which('pwsh'), 'PowerShell unavailable')
class GameLayoutTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='shvnx DLC recovery space ')
        self.addCleanup(self.temporary.cleanup)
        self.card = Path(self.temporary.name) / 'SD'
        self.romfs = self.card / 'atmosphere/contents/0100b00b51230000/romfs'
        self.packs = self.romfs / 'update/switch/dlcpacks'
        for pack in ('mpbiker','mpchristmas2','mpheist','mppatchesng',
                     'patchday1ng','patchday2bng','patchday2ng','patchday3ng'):
            p = self.packs / pack / 'dlc.rpf'; p.parent.mkdir(parents=True)
            p.write_bytes(b'original core archive' * 64)
        self.keep = self.romfs / 'update/update.rpf'; self.keep.write_bytes(b'preserved game configuration')
        self.script = ROOT / 'packaging/Check-Game-Layout.ps1'
        self.shell = shutil.which('powershell') or shutil.which('pwsh')

    def run_helper(self, action='Check', expected=0, card=None, script=None):
        result = subprocess.run([self.shell, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
            str(script or self.script), '-SdRoot', str(card or self.card), '-Action', action, '-Json'],
            capture_output=True, text=True, encoding='utf-8')
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return json.loads(result.stdout) if result.stdout.strip().startswith('{') else None

    def snapshot(self):
        return {p.relative_to(self.card).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in self.card.rglob('*') if p.is_file()}

    def test_check_is_read_only_and_reports_missing_paths(self):
        before = self.snapshot(); result = self.run_helper(expected=2)
        self.assertEqual(len(result['MissingOptional']), 17)
        self.assertTrue(result['RecoveryAllowed']); self.assertEqual(before, self.snapshot())

    def test_recovery_preserves_existing_data_and_is_idempotent(self):
        real = self.packs / 'mpassault/dlc.rpf'; real.parent.mkdir(); real.write_bytes(b'real optional archive' * 100)
        custom = self.packs / 'custom-pack/dlc.rpf'; custom.parent.mkdir(); custom.write_bytes(b'custom DLC')
        before = self.snapshot(); result = self.run_helper('Recover')
        self.assertEqual(len(result['Placeholders']), 16); self.assertFalse(result['MissingOptional'])
        self.assertTrue(all(self.snapshot().get(n) == h for n,h in before.items()))
        record = json.loads(Path(result['Record']).read_text(encoding='utf-8-sig'))
        self.assertEqual(record['State'], 'completed'); self.assertEqual(len(record['Files']), 16)
        after = self.snapshot(); self.run_helper('Recover'); self.assertEqual(after, self.snapshot())

    def test_single_missing_path_retains_array_shape(self):
        result = self.run_helper('Recover'); missing = result['Placeholders'][0]
        (self.romfs / missing).unlink(); check = self.run_helper(expected=2)
        self.assertEqual(check['MissingOptional'], [missing]); self.run_helper('Recover')

    def test_missing_core_is_never_stubbed(self):
        (self.packs / 'mpbiker/dlc.rpf').unlink(); before = self.snapshot()
        result = self.run_helper(expected=1); self.assertFalse(result['RecoveryAllowed'])
        self.run_helper('Recover', expected=1); self.assertEqual(before, self.snapshot())

    def test_core_placeholder_is_not_accepted_as_original_archive(self):
        (self.packs / 'mpheist/dlc.rpf').write_bytes(bytes(512)); before = self.snapshot()
        self.run_helper('Recover', expected=1); self.assertEqual(before, self.snapshot())

    def test_existing_directory_or_truncated_optional_is_preserved(self):
        for directory in (True, False):
            p = self.packs / 'mpassault/dlc.rpf'; p.parent.mkdir(exist_ok=True)
            if directory: p.mkdir()
            else: p.write_bytes(b'short')
            before = self.snapshot(); self.run_helper('Recover', expected=1)
            self.assertEqual(before, self.snapshot())
            if directory: p.rmdir()

    def test_empty_or_wrong_root_does_not_create_a_game_installation(self):
        wrong = self.card / 'switch'; wrong.mkdir(); before = self.snapshot()
        self.run_helper('Recover', expected=1, card=wrong); self.assertEqual(before, self.snapshot())

    def test_path_links_are_rejected_without_following_them(self):
        outside = Path(self.temporary.name) / 'outside'; outside.mkdir()
        link = self.packs / 'mpassault'
        try: link.symlink_to(outside, target_is_directory=True)
        except OSError: self.skipTest('Creating symlinks requires developer mode or permissions')
        self.run_helper('Recover', expected=1); self.assertFalse(list(outside.iterdir()))

    def test_profile_is_self_contained_in_the_install_package(self):
        package = Path(self.temporary.name) / 'runtime-kit'; stage(package)
        for name in ('Check-Game-Layout.ps1','Check-Game-Layout.cmd','GAME-STARTUP.md'):
            self.assertTrue((package/name).is_file())
        self.run_helper('Recover', script=package/'Check-Game-Layout.ps1')

    def test_no_runtime_or_menu_module_is_required_for_recovery(self):
        self.assertFalse((self.romfs.parent/'exefs').exists())
        self.run_helper('Recover'); self.assertFalse((self.romfs.parent/'exefs').exists())


if __name__ == '__main__':
    unittest.main(verbosity=2)
