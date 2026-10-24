#!/usr/bin/env python3
"""Build and package ScriptHookV-NX for Nintendo Switch."""
import os
import sys
import shutil
import argparse
import subprocess
import json
import hashlib
from pathlib import Path
from generate_bindings import generate, Image, DEFAULT_PROFILE
from build_plugin import toolchain, tool
from nso import decode_nso, segments
from runtime_limits import build_defaults, definitions, validate
from engine_bindings import generate_engine
from sd_permissions import bundle, copy_distribution, stage as stage_sd_access

def main():
    parser = argparse.ArgumentParser(description="Build ScriptHookV-NX NSO")
    parser.add_argument("--slot", type=str, default="subsdk5",
                        help="Target subsdk slot name in exefs (default: subsdk5, keeping subsdk4 for Self Radio NX)")
    parser.add_argument('--output', required=True, type=Path, help='New output directory; existing builds are preserved')
    parser.add_argument('--menu', type=Path, help='Exact paired Switch ragemenu.nsc for cached-handler repair')
    parser.add_argument('--devkitpro', type=Path, help='devkitPro root (or DEVKITPRO environment variable)')
    parser.add_argument('--hactool', type=Path, help='Optional additional independent NSO verification tool')
    parser.add_argument('--target-profile', type=Path, default=DEFAULT_PROFILE, help='Audited target profile directory')
    parser.add_argument('--target-main', type=Path, help='Optional extracted main NSO for rederivation; requires --target-sdk')
    parser.add_argument('--target-sdk', type=Path, help='Optional extracted SDK NSO for rederivation; requires --target-main')
    parser.add_argument('--resident-limit', type=int, help='Compiled default resident NRO limit (8 unless the header is edited)')
    parser.add_argument('--script-slots', type=int, help='Compiled default cooperative script slots (16 unless the header is edited)')
    parser.add_argument('--plugin-memory-mib', type=int, help='Compiled default plugin accounting budget in MiB (8 unless the header is edited)')
    args = parser.parse_args()

    project_root = Path(__file__).resolve().parents[1]
    bundle(project_root)
    overrides = {key: getattr(args, key) for key in ('resident_limit', 'script_slots', 'plugin_memory_mib')
                 if getattr(args, key) is not None}
    validate(overrides, definitions(project_root/'include/runtime_defaults.h'))
    if args.slot != 'subsdk5':
        parser.error('The supported runtime uses subsdk5; subsdk4 is reserved for Self Radio')
    if args.menu is not None and (args.target_main is None or args.target_sdk is None):
        parser.error('--menu is research-only and requires both --target-main and --target-sdk')
    if (args.target_main is None) != (args.target_sdk is None):
        parser.error('Provide both --target-main and --target-sdk')
    compiler, _ = toolchain(project_root, args.devkitpro)
    devkit_root = compiler.parents[2]
    readelf = tool(devkit_root, 'devkitA64/bin/aarch64-none-elf-readelf')
    elf2nso = tool(devkit_root, 'tools/bin/elf2nso')
    hactool = args.hactool.resolve() if args.hactool else None
    if hactool is not None and not hactool.is_file():
        parser.error('--hactool does not point to a file')
    build_dir = args.output.resolve()
    if build_dir.exists():
        parser.error('Use a fresh output directory; existing builds are preserved')
    if any(build_dir.is_relative_to(project_root/folder) for folder in ('source', 'include', 'sdk', 'tools', 'targets')):
        parser.error('Output must be outside the source/header/tool/profile trees being snapshotted')
    # Validate the selected target before creating any output directory.
    import tempfile
    with tempfile.TemporaryDirectory(prefix='shvnx-profile-') as temporary:
        generated = Path(temporary)/'generated'
        generate(generated, args.menu.resolve() if args.menu else None,
                 args.target_main, args.target_sdk, args.target_profile)
        generate_engine(generated,args.target_profile.resolve())
        build_dir.mkdir(parents=True)
        shutil.copytree(generated, build_dir/'generated')
    for folder in ('source', 'include', 'sdk', 'tools', 'targets'):
        shutil.copytree(project_root/folder, build_dir/folder, ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('LICENSE', 'NOTICE', 'THIRD-PARTY-NOTICES.txt'):
        shutil.copy2(project_root/name, build_dir/name)
    copy_distribution(build_dir, project_root)
    limits = build_defaults(build_dir/'include/runtime_defaults.h', overrides)

    print("========================================")
    print("   Building ScriptHookV-NX (AArch64)   ")
    print(f"   Target Slot: {args.slot}")
    print("========================================")

    common_flags = [
        '-march=armv8-a+crc', '-mtune=cortex-a57', '-mtp=soft', '-fPIC', '-O2', '-g',
        '-ffreestanding', '-fno-builtin', '-fno-stack-protector', '-fno-exceptions', '-fno-rtti',
        '-fvisibility=hidden', '-fno-threadsafe-statics', '-mno-outline-atomics',
        '-ffunction-sections', '-fdata-sections', '-Wall', '-Wextra', '-Werror',
        '-fstack-usage',
        '-fdebug-prefix-map=' + str(build_dir) + '=.',
        '-ffile-prefix-map=' + str(build_dir) + '=.',
        '-fdebug-prefix-map=' + str(devkit_root) + '=/devkitPro',
        '-std=c++17',
        f"-I{build_dir / 'include'}",
        f"-I{build_dir / 'source'}"
    ]
    common_flags += ['-I' + str(build_dir / 'generated')]

    sources = [
        project_root / 'source/core/start.s',
        project_root / 'source/core/memory.cpp',
        project_root / 'source/core/storage.cpp',
        project_root / 'source/core/runtime_config.cpp',
        project_root / 'source/core/engine_api.cpp',
        project_root / 'source/core/main.cpp',
        project_root / 'source/script/native_table.cpp',
        project_root / 'source/script/native_manifest.cpp',
        project_root / 'source/script/native_caller.cpp',
        project_root / 'source/script/script_manager.cpp',
        project_root / 'source/script/scr_thread.cpp',
        project_root / 'source/bridge/persistence.cpp',
        project_root / 'source/bridge/public_natives.cpp',
        project_root / 'source/plugins/plugin_loader.cpp',
        project_root / 'source/plugins/plugin_callbacks.cpp',
        project_root / 'source/plugins/plugin_textures.cpp',
    ]

    objects = []
    for src in sources:
        src = build_dir / src.relative_to(project_root)
        obj = build_dir / (src.name + '.o')
        cmd = [str(compiler)] + common_flags + ['-c', str(src), '-o', str(obj)]
        print(f"  [CC] {src.relative_to(build_dir)}")
        res = subprocess.run(cmd, cwd=build_dir, capture_output=True, text=True)
        if res.returncode != 0:
            print(f"Compilation failed for {src}:")
            print(res.stdout)
            print(res.stderr)
            sys.exit(1)
        objects.append(obj)

    # Link ELF
    elf_file = build_dir / 'scripthook-nx.elf'
    link_script = build_dir / 'source/core/link.ld'
    link_cmd = [
        str(compiler), *common_flags,
        '-nostdlib', '-nostartfiles', '-shared',
        *[str(obj) for obj in objects],
        f'-Wl,-T,{link_script}',
        '-Wl,-soname,scripthookv-nx',
        '-Wl,--build-id=sha1,--hash-style=both,--nx-module-name',
        '-Wl,-init=scripthook_module_init,-z,defs,-Bsymbolic',
        f'-Wl,-Map,{build_dir / "scripthook-nx.map"}',
        '-o', str(elf_file)
    ]

    print("  [LD] Linking scripthook-nx.elf")
    res = subprocess.run(link_cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("Linking failed:")
        print(res.stdout)
        print(res.stderr)
        sys.exit(1)

    # Convert ELF to target NSO (e.g. subsdk5)
    nso_slot_file = build_dir / args.slot
    nso_named_file = build_dir / 'scripthook-nx.nso'

    print(f"  [NSO] Converting to Nintendo Switch NSO: {args.slot}")
    res = subprocess.run([str(elf2nso), str(elf_file), str(nso_slot_file)], capture_output=True, text=True)
    if res.returncode != 0:
        print("elf2nso failed:")
        print(res.stderr)
        sys.exit(1)

    # Also emit scripthook-nx.nso
    shutil.copy2(nso_slot_file, nso_named_file)

    # Always verify/decode all NSO segments using the self-contained reader.
    # hactool is optional extra validation, not a private workspace dependency.
    uncompressed_nso = build_dir / 'scripthook.uncompressed.nso'
    raw = nso_slot_file.read_bytes()
    layout = segments(raw)
    if not all(segment['hash_required'] for segment in layout):
        raise RuntimeError('Produced NSO omitted required segment hashes')
    uncompressed_nso.write_bytes(decode_nso(raw))
    image = Image(uncompressed_nso)
    print('  [NSO] All three segment hashes verified; dynamic imports/relocations checked.')
    if hactool is not None:
        val_cmd = [str(hactool), '-t', 'nso', '-y', '--disablekeywarns', str(nso_slot_file)]
        res = subprocess.run(val_cmd, capture_output=True, text=True)
        if res.returncode != 0 or res.stdout.count('(GOOD)') != 3:
            raise RuntimeError('hactool segment validation failed: ' + res.stdout + res.stderr)
        print('  [HACTOOL] Independent verification: all three segments GOOD.')
    if any(r['type'] != 1027 for r in image.relocations):
        raise RuntimeError('Unexpected non-relative module relocation')
    if any(symbol['name'] and not symbol['section'] for symbol in image.symbols):
        raise RuntimeError('Unresolved dynamic module imports')
    listing = subprocess.run([str(readelf), '-aW', str(elf_file)], capture_output=True, text=True, check=True)
    (build_dir / 'elf-verification.txt').write_text(listing.stdout, encoding='utf-8')
    sd_access = stage_sd_access(build_dir/'package', build_dir)
    shutil.copy2(nso_slot_file, build_dir/'package/atmosphere/contents/0100b00b51230000/exefs'/args.slot)
    report = dict(compiled=True, packaged=True, switch_runtime_verified=False, sd_access=sd_access,
                  module_sha256=hashlib.sha256(nso_slot_file.read_bytes()).hexdigest(),
                  source_sha256={p.relative_to(build_dir).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                                 for folder in ('source', 'include', 'sdk', 'tools', 'generated', 'targets', 'distribution')
                                 for p in (build_dir / folder).rglob('*') if p.is_file()},
                  segment_hashes_verified=True, unresolved_imports=[], plugin_execution_enabled=True,
                  target_build=json.loads((build_dir/'generated/bindings.json').read_text())['main_build_id'][:40],
                  target_source='rederived' if args.target_main else 'pinned_profile',
                  hactool_verified=hactool is not None,
                  segment_sha256={segment['name']: hashlib.sha256(image.read(segment['memory_offset'],segment['size'])).hexdigest()
                                  for segment in image.layout},
                  toolchain=dict(compiler=str(compiler), elf2nso=str(elf2nso), readelf=str(readelf)),
                  external_plugin_loading_enabled=True,
                  runtime_scheduler_enabled=True, startup_scripts=0,
                  runtime_max_scripts=limits['SHVNX_DEFAULT_SCRIPT_SLOTS'], runtime_stack_bytes=65536,
                  plugin_resident_limit=limits['SHVNX_DEFAULT_RESIDENT_PLUGINS'],
                  plugin_memory_budget_bytes=limits['SHVNX_DEFAULT_PLUGIN_MEMORY_MIB']*1024*1024,
                  runtime_limits=limits, runtime_config_path='switch/gtav-scripthook/runtime.ini')
    (build_dir / 'build-manifest.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

    slot_size = os.path.getsize(nso_slot_file)
    print("========================================")
    print("   SUCCESS! ScriptHookV-NX built!      ")
    print(f"   Slot Output:  {nso_slot_file} ({slot_size} bytes)")
    print(f"   Named Output: {nso_named_file}")
    print("----------------------------------------")
    print("   Multi-Mod Coexistence:")
    print("     exefs/subsdk4 -> Self Radio NX (UserMusic)")
    print(f"     exefs/{args.slot} -> ScriptHookV-NX")
    print("========================================")

if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        raise SystemExit('Build failed: '+str(error))
