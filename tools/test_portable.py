"""Standard-library regressions runnable from a clean public checkout; no game files."""
import hashlib
import json
from pathlib import Path
import shutil
import struct
import tempfile
import unittest

from build_plugin import tool, toolchain
from build_mod import inputs
from create_mod import create
from generate_bindings import DEFAULT_PROFILE, profile_bindings
from inspect_mod import inspect_bindings
from native_query import search
from nso import decode_nso, lz4_block, segments
from report_summary import summarize
from runtime_limits import definitions, build_defaults, render, validate, HEADER

class PortableTests(unittest.TestCase):
    def test_runtime_default_definitions(self):
        values=definitions()
        # Intentional edits to build defaults must not break the portable suite.
        from runtime_limits import KEYS
        validate({key:values[macros[0]] for key,macros in KEYS.items()},values)
    def test_runtime_partial_config_generation(self):
        text=render({'script_slots':24})
        self.assertIn('script_slots=24\n',text)
        self.assertNotIn('resident_limit=',text)
        self.assertNotIn('plugin_memory_mib=',text)
    def test_runtime_invalid_config_values(self):
        values=definitions()
        for settings in ({'resident_limit':values['SHVNX_MAX_RESIDENT_PLUGINS']+1},
                         {'script_slots':values['SHVNX_MAX_SCRIPT_SLOTS']+1},
                         {'plugin_memory_mib':values['SHVNX_MAX_PLUGIN_MEMORY_MIB']+1},
                         {'script_slots':0},{'resident_limit':True},{'script_slots':'24'},
                         {'plugin_memory_mib':1.5},{'typo':8},[]):
            with self.subTest(settings=settings), self.assertRaises(ValueError): validate(settings)
    def test_runtime_build_overrides_snapshot_only(self):
        before=HEADER.read_bytes()
        with tempfile.TemporaryDirectory() as name:
            header=Path(name)/'runtime_defaults.h';shutil.copy2(HEADER,header)
            values=build_defaults(header,{'resident_limit':12,'script_slots':24,'plugin_memory_mib':16})
            self.assertEqual(values['SHVNX_DEFAULT_SCRIPT_SLOTS'],24)
            self.assertEqual(values['SHVNX_MAX_SCRIPT_SLOTS'],32)
            self.assertEqual(HEADER.read_bytes(),before)
    def test_runtime_project_config_validation(self):
        with tempfile.TemporaryDirectory() as name:
            project=Path(name)/'mod';create('hello','test-mod',project)
            path=project/'project.json';data=json.loads(path.read_text())
            data['runtime_limits']={'resident_limit':12,'script_slots':24}
            path.write_text(json.dumps(data));self.assertEqual(inputs(path)[0]['runtime_limits'],data['runtime_limits'])
            data['runtime_limits']={'script_slots':9000};path.write_text(json.dumps(data))
            with self.assertRaises(ValueError):inputs(path)
    def test_runtime_config_cli_preserves_existing_file(self):
        import subprocess,sys
        with tempfile.TemporaryDirectory() as name:
            output=Path(name)/'runtime.ini'
            command=[sys.executable,str(HEADER.parent.parent/'tools/configure_runtime.py'),'--script-slots','24','--output',str(output)]
            self.assertEqual(subprocess.run(command,capture_output=True).returncode,0)
            before=output.read_bytes()
            self.assertNotEqual(subprocess.run(command,capture_output=True).returncode,0)
            self.assertEqual(output.read_bytes(),before)
    def test_resident_capacity_diagnostic_uses_reported_limit(self):
        for limit in (4,8):
            with self.subTest(limit=limit), tempfile.TemporaryDirectory() as name:
                report=Path(name)/'loader-report.txt'
                report.write_text('ScriptHookV-NX capacity test\nresident_limit='+hex(limit)+'\nplugin=extra.nro\nerror=0x10015\n')
                self.assertIn(str(limit)+' resident modules',summarize(report)['notes'][0])
    def test_resident_capacity_diagnostic_tolerates_older_or_invalid_report(self):
        for field in ('','resident_limit=invalid\n'):
            with self.subTest(field=field), tempfile.TemporaryDirectory() as name:
                report=Path(name)/'loader-report.txt'
                report.write_text('ScriptHookV-NX old report\n'+field+'plugin=extra.nro\nerror=0x10015\n')
                self.assertIn('resident module limit already reached',summarize(report)['notes'][0])
    def test_timing_summary_preserves_plugin_attribution_and_units(self):
        with tempfile.TemporaryDirectory() as name:
            report=Path(name)/'timing.txt';report.write_text('ScriptHookV-NX plugin timing snapshot\nunits=microseconds\nplugin=slow.nro\nscript_calls=2\nscript_total_us=6000\nscript_worst_us=4000\nplugin=quick.nro\nscript_calls=100\nscript_total_us=10000\nscript_worst_us=150\n')
            result=summarize(report)
            self.assertIn('slow.nro, 4000 us elapsed',result['notes'][0]);self.assertIn('average 3000 us',result['notes'][0])
            self.assertIn('not FPS or isolated CPU time',result['notes'][0])
    def test_lz4_literals(self):
        self.assertEqual(lz4_block(b'\x30abc', 3), b'abc')
    def test_lz4_overlapping_reference(self):
        self.assertEqual(lz4_block(b'\x11a\x01\0', 6), b'aaaaaa')
    def test_lz4_extended_literal(self):
        self.assertEqual(lz4_block(b'\xf0\x05'+b'x'*20, 20), b'x'*20)
    def test_lz4_invalid_sequences(self):
        for data, size in ((b'\x30ab', 3), (b'\x10a\0\0', 5), (b'\x10a\x02\0', 5),
                           (b'\xf0\xff', 20), (b'\x11a\x01\0', 5), (b'\x10a', 2)):
            with self.subTest(data=data):
                with self.assertRaises(ValueError): lz4_block(data, size)
    def test_nso_segment_hashes_and_corruption(self):
        header = bytearray(0x100); header[:4] = b'NSO0'; struct.pack_into('<I', header, 0xc, 0x38)
        data = bytearray(header)
        for i in range(3):
            block = bytes([i+1])*16
            struct.pack_into('<III', data, 0x10+16*i, len(data), i*0x1000, len(block))
            data[0xa0+32*i:0xc0+32*i] = hashlib.sha256(block).digest(); data.extend(block)
        self.assertEqual(decode_nso(bytes(data)), bytes(data))
        self.assertEqual(len(segments(data)), 3)
        data[-1] ^= 1
        with self.assertRaisesRegex(ValueError, 'hash mismatch'): decode_nso(bytes(data))
    def test_compressed_nso_segment(self):
        header = bytearray(0x100); header[:4] = b'NSO0'; struct.pack_into('<I', header, 0xc, 0x3f)
        data = bytearray(header)
        for i in range(3):
            raw = b'a'*6; packed = b'\x11a\x01\0'
            struct.pack_into('<III', data, 0x10+16*i, len(data), i*0x1000, len(raw))
            struct.pack_into('<I', data, 0x60+4*i, len(packed))
            data[0xa0+32*i:0xc0+32*i] = hashlib.sha256(raw).digest(); data.extend(packed)
        decoded = decode_nso(bytes(data))
        self.assertTrue(all(not s['compressed'] for s in segments(decoded)))
        for segment in segments(decoded): self.assertEqual(decoded[segment['file_offset']:segment['file_offset']+6], b'a'*6)
    def test_pinned_profile(self):
        with tempfile.TemporaryDirectory(prefix='shvnx profile ') as name:
            output = Path(name)/'generated'
            result = profile_bindings(output)
            self.assertIsNone(result['menu_sha256'])
            self.assertEqual((output/'bindings.hpp').read_bytes(), (DEFAULT_PROFILE/'bindings.hpp').read_bytes())
    def test_tampered_profile_rejected_before_output(self):
        with tempfile.TemporaryDirectory() as name:
            profile, output = Path(name)/'profile', Path(name)/'out'
            shutil.copytree(DEFAULT_PROFILE, profile)
            (profile/'bindings.hpp').write_text('corrupt')
            with self.assertRaisesRegex(ValueError, 'hash mismatch'): profile_bindings(output, profile)
            self.assertFalse(output.exists())
    def test_tool_names_without_exe(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name); path = root/'tools/bin/elf2nso'; path.parent.mkdir(parents=True); path.touch()
            self.assertEqual(tool(root, 'tools/bin/elf2nso'), path)
    def test_tool_names_with_exe(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name); path = root/'tools/bin/elf2nso.exe'; path.parent.mkdir(parents=True); path.touch()
            self.assertEqual(tool(root, 'tools/bin/elf2nso'), path)
    def test_missing_explicit_toolchain(self):
        with tempfile.TemporaryDirectory() as name:
            with self.assertRaises(FileNotFoundError): toolchain(Path(name), Path(name))
    def test_project_generation_and_preservation(self):
        with tempfile.TemporaryDirectory(prefix='shvnx project ') as name:
            project = Path(name)/'mod'
            create('saved-counter', 'my-example', project)
            data, sources, _ = inputs(project/'project.json')
            self.assertEqual(data['kind'], 'nro'); self.assertEqual(len(sources), 1)
            with self.assertRaises(FileExistsError): create('hello', 'my-example', project)
    def test_path_traversal_rejected(self):
        with tempfile.TemporaryDirectory() as name:
            project = Path(name)/'project'; create('hello', 'my-example', project)
            config = json.loads((project/'project.json').read_text()); config['sources'] = ['../escape.cpp']
            (project/'project.json').write_text(json.dumps(config))
            with self.assertRaises(ValueError): inputs(project/'project.json')
    def test_native_index_matches_header(self):
        rows = search('GET_ENTITY_COORDS', registered=True)
        self.assertTrue(any(row['hash']=='0x6b62510f212526dc' for row in rows))
    def test_non_object_project_rejected(self):
        with tempfile.TemporaryDirectory() as name:
            project = Path(name)/'project.json'; project.write_text('[]')
            with self.assertRaisesRegex(ValueError, 'format 1'): inputs(project)
    def test_binding_crc_rejected(self):
        body = bytes(48)
        with self.assertRaisesRegex(ValueError, 'CRC'): inspect_bindings(struct.pack('<4I', 0x424e4853, 1, 1, 0)+body)

if __name__ == '__main__':
    unittest.main(verbosity=2)
