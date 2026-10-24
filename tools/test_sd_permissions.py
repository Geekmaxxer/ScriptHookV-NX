"""Portable permission/packaging regression checks; no live SD files are used."""
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
from sd_permissions import ROOT, NPDM_PATH, SD_CARD, bundle, inspect, patch, reverse, stage, copy_distribution

class SdPermissionsTests(unittest.TestCase):
    def setUp(self):
        self.enabled=(bundle()/NPDM_PATH).read_bytes()
        self.disabled=bytearray(self.enabled)
        for m in inspect(self.disabled)['masks']:struct.pack_into('<Q',self.disabled,m['offset'],m['flags']&~SD_CARD)
        self.disabled=bytes(self.disabled)
    def test_minimal_reversible_change(self):
        after,proof=patch(self.disabled)
        self.assertEqual(after,self.enabled)
        self.assertEqual(len(proof['changed_offsets']),2)
        self.assertEqual(reverse(after,proof),self.disabled)
    def test_already_enabled_is_idempotent(self):self.assertEqual(patch(self.enabled)[0],self.enabled)
    def test_custom_settings_and_permissions_preserved(self):
        data=bytearray(self.disabled);data[0x20]^=0x10
        for m in inspect(data)['masks']:struct.pack_into('<Q',data,m['offset'],m['flags']|(1<<8))
        after,_=patch(data);allowed={m['offset']+2 for m in inspect(data)['masks']}
        self.assertTrue(all(a==b or i in allowed for i,(a,b) in enumerate(zip(data,after))))
    def test_each_mask_is_required(self):
        for keep in (0,1):
            data=bytearray(self.disabled);m=inspect(data)['masks'][keep]
            struct.pack_into('<Q',data,m['offset'],m['flags']|SD_CARD)
            self.assertEqual(patch(data)[0],self.enabled)
    def test_wrong_title_rejected(self):
        data=bytearray(self.enabled);aci=struct.unpack_from('<I',data,0x70)[0];struct.pack_into('<Q',data,aci+0x10,1)
        with self.assertRaises(ValueError):inspect(data)
    def test_bounds_and_fs_version_rejected(self):
        for at,value in ((0x70,0xfffffff0),(inspect(self.enabled)['masks'][0]['offset']-4,2)):
            data=bytearray(self.enabled);struct.pack_into('<I',data,at,value)
            with self.assertRaises(ValueError):inspect(data)
    def test_staging_and_source_copy_contain_verified_metadata(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);proof=stage(root/'install');copy_distribution(root/'sdk')
            self.assertTrue(proof['sd_access_included'])
            self.assertEqual((root/'install'/NPDM_PATH).read_bytes(),self.enabled)
            self.assertEqual((root/'sdk/distribution'/NPDM_PATH).read_bytes(),self.enabled)
    def test_different_staged_file_is_not_overwritten(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);p=root/NPDM_PATH;p.parent.mkdir(parents=True);p.write_bytes(self.disabled)
            with self.assertRaises(ValueError):stage(root)
            self.assertEqual(p.read_bytes(),self.disabled)
    def test_missing_bundle_fails(self):
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaises(FileNotFoundError):bundle(Path(d))
    def test_powershell_fresh_custom_and_already_enabled(self):
        ps=shutil.which('powershell') or shutil.which('pwsh')
        if not ps:self.skipTest('PowerShell not installed')
        script=bundle()/'Install-SD-Access.ps1'
        with tempfile.TemporaryDirectory(prefix='NPDM test with spaces ') as d:
            root=Path(d);(root/'switch').mkdir()
            def run(*args,good=True):
                p=subprocess.run([ps,'-NoProfile','-ExecutionPolicy','Bypass','-File',str(script),'-SdRoot',str(root),*args],capture_output=True,text=True)
                self.assertEqual(p.returncode==0,good,p.stdout+p.stderr)
            run('-CheckOnly',good=False);run();self.assertEqual((root/NPDM_PATH).read_bytes(),self.enabled)
            custom=bytearray(self.disabled);custom[0x20]^=0x10;(root/NPDM_PATH).write_bytes(custom)
            run();self.assertEqual((root/NPDM_PATH).read_bytes(),patch(custom)[0]);self.assertEqual(len(list((root/'switch/gtav-scripthook/npdm-backups').glob('*.npdm'))),1)
            before=(root/NPDM_PATH).stat().st_mtime_ns;run();run('-CheckOnly');self.assertEqual((root/NPDM_PATH).stat().st_mtime_ns,before)
            (root/NPDM_PATH).write_bytes(b'invalid');run(good=False);self.assertEqual((root/NPDM_PATH).read_bytes(),b'invalid')

if __name__=='__main__':unittest.main(verbosity=2)
