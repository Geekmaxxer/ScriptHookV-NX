"""Exercise capacity-profile preservation on disposable SD fixtures only."""
import bootstrap
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
POWERSHELL=shutil.which('powershell.exe') or shutil.which('pwsh')


@unittest.skipUnless(POWERSHELL,'PowerShell is required for the packaged Windows helper')
class CapacityProfileTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.base=Path(self.temp.name).resolve();self.card=self.base/'card';self.kit=self.base/'kit';self.kit.mkdir()
        shutil.copy2(ROOT/'examples/capacity-acceptance/Select-Profile.ps1',self.kit/'Select-Profile.ps1')
        (self.kit/'profiles').mkdir()
        profiles={'Defaults':b'# compiled defaults\n','Explicit':b'script_slots=48\n','Automatic':b'script_slots=0\n'}
        for name,data in profiles.items():(self.kit/'profiles'/(name.lower()+'.ini')).write_bytes(data)
        self.module=self.card/'atmosphere/contents/0100b00b51230000/exefs/subsdk5';self.module.parent.mkdir(parents=True);self.module.write_bytes(b'fixture runtime')
        self.plugin=self.card/'switch/gtav-scripthook/plugins/capacity-acceptance.nro.manual';self.plugin.parent.mkdir(parents=True);self.plugin.write_bytes(b'fixture NRO')
        self.config=self.card/'switch/gtav-scripthook/runtime.ini';self.original=b'\xef\xbb\xbf# custom\r\nresident_limit=12\r\n';self.config.write_bytes(self.original)
        self.backup=self.card/'switch/gtav-scripthook/settings/capacity-acceptance'
        self.manifest={'format':1,'runtime_sha256':self.digest(self.module),'plugin_sha256':self.digest(self.plugin),
            'profiles':{name:self.digest(self.kit/'profiles'/(name.lower()+'.ini')) for name in profiles}}
        (self.kit/'profile-manifest.json').write_text(json.dumps(self.manifest),encoding='utf-8')
    @staticmethod
    def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
    def select(self,mode,success=True):
        run=subprocess.run([POWERSHELL,'-NoLogo','-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',str(self.kit/'Select-Profile.ps1'),'-SdRoot',str(self.card),'-Profile',mode],capture_output=True,text=True)
        self.assertEqual(run.returncode==0,success,run.stdout+run.stderr)
        return run
    def test_switch_and_restore_preserve_exact_original_and_other_files(self):
        self.backup.mkdir(parents=True);keep=self.backup/'keep.txt';keep.write_bytes(b'preserved')
        for mode in ('Defaults','Explicit','Automatic'):
            self.select(mode);self.assertEqual(self.config.read_bytes(),(self.kit/'profiles'/(mode.lower()+'.ini')).read_bytes())
            self.assertEqual((self.backup/'runtime.ini.original').read_bytes(),self.original)
        self.select('Restore');self.select('Restore');self.assertEqual(self.config.read_bytes(),self.original);self.assertEqual(keep.read_bytes(),b'preserved')
    def test_original_absence_is_restored(self):
        self.config.unlink();self.select('Explicit');self.select('Restore');self.assertFalse(self.config.exists());self.assertFalse((self.backup/'runtime.ini.original').exists())
    def test_original_empty_file_is_distinct_from_absence(self):
        self.config.write_bytes(b'');self.select('Automatic');self.select('Restore');self.assertTrue(self.config.exists());self.assertEqual(self.config.read_bytes(),b'')
    def test_wrong_runtime_refuses_before_backup_or_configuration_changes(self):
        self.module.write_bytes(b'wrong runtime');self.select('Explicit',False);self.assertEqual(self.config.read_bytes(),self.original);self.assertFalse(self.backup.exists())
    def test_wrong_probe_refuses_before_configuration_changes(self):
        self.plugin.write_bytes(b'wrong NRO');self.select('Explicit',False);self.assertEqual(self.config.read_bytes(),self.original);self.assertFalse(self.backup.exists())
    def test_modified_profile_refuses_before_backup(self):
        (self.kit/'profiles/explicit.ini').write_bytes(b'changed');self.select('Explicit',False);self.assertFalse(self.backup.exists());self.assertEqual(self.config.read_bytes(),self.original)
    def test_restore_does_not_depend_on_current_runtime_or_probe(self):
        self.select('Explicit');self.module.unlink();self.plugin.unlink();self.select('Restore');self.assertEqual(self.config.read_bytes(),self.original)
    def test_unrelated_configuration_edits_are_preserved(self):
        self.select('Explicit');self.config.write_bytes(b'new external settings');self.select('Automatic',False);self.select('Restore',False);self.assertEqual(self.config.read_bytes(),b'new external settings');self.assertEqual((self.backup/'runtime.ini.original').read_bytes(),self.original)
    def test_damaged_backup_never_replaces_configuration(self):
        self.select('Explicit');current=self.config.read_bytes();(self.backup/'runtime.ini.original').write_bytes(b'corrupt');self.select('Restore',False);self.assertEqual(self.config.read_bytes(),current)
    def test_orphaned_backup_is_preserved(self):
        self.backup.mkdir(parents=True);(self.backup/'runtime.ini.original').write_bytes(b'orphan');self.select('Explicit',False);self.assertEqual(self.config.read_bytes(),self.original);self.assertEqual((self.backup/'runtime.ini.original').read_bytes(),b'orphan')
    def test_temporary_config_is_never_overwritten(self):
        temp=self.config.with_name('runtime.ini.capacity-next');temp.write_bytes(b'preserve interrupted write');self.select('Explicit',False);self.assertEqual(self.config.read_bytes(),self.original);self.assertEqual(temp.read_bytes(),b'preserve interrupted write')
    def test_missing_backup_and_malformed_journal_are_refused(self):
        self.select('Explicit');current=self.config.read_bytes();(self.backup/'runtime.ini.original').unlink();self.select('Restore',False);(self.backup/'profile-backup.json').write_bytes(b'{bad JSON');self.select('Restore',False);self.assertEqual(self.config.read_bytes(),current)
    def test_phase_capture_preserves_reports_and_excludes_account_files(self):
        self.select('Defaults');(self.card/'switch/gtav-scripthook/loader-report.txt').write_bytes(b'default refusal')
        self.backup.joinpath('status.txt').write_bytes(b'probe report');private=self.card/'switch/gtav-scripthook/settings/user-profile-example';private.mkdir();(private/'profile-name.txt').write_bytes(b'private account')
        for index in (1,8):self.backup.joinpath(f'run-{index}.txt').write_bytes(f'bounded history {index}'.encode())
        self.select('Explicit');captures=list((self.backup/'phase-results').glob('defaults-*'));self.assertEqual(len(captures),1);self.assertEqual((captures[0]/'loader-report.txt').read_bytes(),b'default refusal');self.assertEqual((captures[0]/'status.txt').read_bytes(),b'probe report');self.assertFalse(any(p.name=='profile-name.txt' for p in (self.backup/'phase-results').rglob('*')))
        for index in (1,8):self.assertEqual((captures[0]/f'run-{index}.txt').read_bytes(),f'bounded history {index}'.encode())
    def test_linked_settings_directory_is_refused(self):
        external=self.base/'external';external.mkdir();settings=self.card/'switch/gtav-scripthook/settings'
        command="New-Item -ItemType Junction -Path '"+str(settings).replace("'","''")+"' -Target '"+str(external).replace("'","''")+"' | Out-Null"
        result=subprocess.run([POWERSHELL,'-NoProfile','-NonInteractive','-Command',command],capture_output=True)
        if result.returncode:self.skipTest('Directory junction creation is unavailable')
        self.select('Explicit',False);self.assertEqual(self.config.read_bytes(),self.original);self.assertFalse(list(external.iterdir()))


if __name__=='__main__':unittest.main()
