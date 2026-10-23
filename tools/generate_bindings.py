"""Derive the bridge ABI from the extracted target NSOs, including hook relocations."""
import hashlib
import json
import struct
import shutil
from pathlib import Path

from nso import Image, import_thunks

DEFAULT_PROFILE = Path(__file__).resolve().parents[1] / 'targets/6e3e9b3af5746e74b615bd981080c9271bc226620'
MAIN_SHA = '6d4d4f670d2e7b3ef9a8ac33787859b2d4d80667148132b5d815997c2a196fd5'
FS = {
    'open_directory': '_ZN2nn2fs13OpenDirectoryEPNS0_15DirectoryHandleEPKci',
    'read_directory': '_ZN2nn2fs13ReadDirectoryEPlPNS0_14DirectoryEntryENS0_15DirectoryHandleEl',
    'close_directory': '_ZN2nn2fs14CloseDirectoryENS0_15DirectoryHandleE',
    'open_file': '_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci',
    'read_file': '_ZN2nn2fs8ReadFileEPmNS0_10FileHandleElPvm',
    'close_file': '_ZN2nn2fs9CloseFileENS0_10FileHandleE',
    'create_file': '_ZN2nn2fs10CreateFileEPKcl',
    'set_file_size': '_ZN2nn2fs11SetFileSizeENS0_10FileHandleEl',
    'write_file': '_ZN2nn2fs9WriteFileENS0_10FileHandleElPKvmRKNS0_11WriteOptionE',
    'create_directory': '_ZN2nn2fs15CreateDirectoryEPKc',
    'rename_file': '_ZN2nn2fs10RenameFileEPKcS2_',
    'delete_file': '_ZN2nn2fs10DeleteFileEPKc',
}
SDK = {'mount_sd_debug': '_ZN2nn2fs19MountSdCardForDebugEPKc', 'unmount': '_ZN2nn2fs7UnmountEPKc',
       'disable_thread_abort': '_ZN2nn2fs23ScopedAutoAbortDisablerC1Ev',
       'restore_thread_context': '_ZN2nn2fs25SetCurrentThreadFsContextEPKNS0_9FsContextE'}


def profile_bindings(output, profile=DEFAULT_PROFILE):
    profile = Path(profile).resolve()
    descriptor = json.loads((profile/'profile.json').read_text())
    if descriptor.get('format') != 1 or set(descriptor.get('files', {})) != {'bindings.hpp', 'bindings.json', 'engine-api.json'}:
        raise ValueError('Unsupported target profile descriptor')
    for name, digest in descriptor['files'].items():
        if hashlib.sha256((profile/name).read_bytes()).hexdigest() != digest:
            raise ValueError('Target profile hash mismatch: '+name)
    metadata = json.loads((profile/'bindings.json').read_text())
    if descriptor['build_id'] != metadata['main_build_id'][:40] or metadata.get('menu_sha256') is not None or metadata.get('menu_natives'):
        raise ValueError('Profile must identify its target and must not bind one NSC menu')
    if any(name.startswith('menu_') for name in metadata['constants']):
        raise ValueError('Production profile contains a menu-specific constant')
    output.mkdir(parents=True, exist_ok=True)
    for name in ('bindings.hpp', 'bindings.json'):
        shutil.copy2(profile/name, output/name)
    return metadata


def generate(output, menu=None, main_path=None, sdk_path=None, profile=DEFAULT_PROFILE):
    if main_path is None and sdk_path is None:
        if menu is not None:
            raise ValueError('--menu is a research fixture option; provide --target-main and --target-sdk to rederive it')
        return profile_bindings(output, profile)
    if main_path is None or sdk_path is None:
        raise ValueError('Provide both --target-main and --target-sdk, or neither for the pinned profile')
    main = Image(main_path)
    sdk = Image(sdk_path)
    if hashlib.sha256(main.data).hexdigest() != MAIN_SHA:
        raise ValueError('Unsupported target executable')
    if hashlib.sha256(sdk.data).hexdigest() != '2549a08225392c12b2dee7f14a5c6e49f955aae0dc99bc65628c4b47bf5d7652':
        raise ValueError('Unsupported target SDK')
    thunks = {r['name']: r for r in import_thunks(main)}
    sdk_symbols = {r['name']: r for r in sdk.symbols if r['section'] and r['type'] == 2}
    constants = dict(native_registry=0x3dd6220, native_register=0x2635d08,
                     missing_native=0x26338d0, vm_tick=0x2633e40,
                     vm_slot_base=0x2f9df60, vm_slot_derived=0x2ecac40,
                     program_table=0x3dd5560, program_mutex=0x3dd5bb8,
                     script_tls_thread=0x3b0, script_tls_active=0x3b8)
    # Decode the actual VM context entry. Keep the script guard independent of
    # hand-copied addresses and of any emulator fixture using these bindings.
    adrp_at, store_at = 0x2633eac, 0x2633eb8
    adrp = struct.unpack('<I', main.read(adrp_at, 4))[0]
    store = struct.unpack('<I', main.read(store_at, 4))[0]
    if adrp & 0x9f00001f != 0x90000009 or store & 0xffc003ff != (0xf9000000 | (9 << 5) | 28):
        raise ValueError('VM active-script setup instruction changed')
    immediate = ((adrp >> 5) & 0x7ffff) << 2 | (adrp >> 29) & 3
    if immediate & (1 << 20): immediate -= 1 << 21
    constants['active_script_thread'] = (adrp_at & ~4095) + (immediate << 12) + ((store >> 10) & 4095) * 8
    # The interpreter also publishes thread+0xd4 as its active auxiliary state.
    aux_adrp, aux_store = struct.unpack('<I', main.read(0x2633e68, 4))[0], struct.unpack('<I', main.read(0x2633e7c, 4))[0]
    if aux_adrp & 0x9f00001f != 0x90000009 or aux_store & 0xffc003ff != (0xf9000000 | (9 << 5) | 8):
        raise ValueError('VM auxiliary context instruction changed')
    imm = ((aux_adrp >> 5) & 0x7ffff) << 2 | (aux_adrp >> 29) & 3
    if imm & (1 << 20): imm -= 1 << 21
    constants['active_script_aux'] = (0x2633e68 & ~4095) + (imm << 12) + ((aux_store >> 10) & 4095) * 8
    # GET_FRAME_COUNT's real handler reads this global directly. Derive it
    # from the target instructions so the scheduler's same-frame fast path
    # never depends on a separately maintained address.
    frame_at = 0x1020480
    frame_adrp = struct.unpack('<I', main.read(frame_at, 4))[0]
    frame_load = struct.unpack('<I', main.read(frame_at + 8, 4))[0]
    if frame_adrp & 0x9f00001f != 0x90000008 or frame_load & 0xffc003ff != (0xb9400000 | (8 << 5) | 8):
        raise ValueError('GET_FRAME_COUNT global-load instruction changed')
    imm = ((frame_adrp >> 5) & 0x7ffff) << 2 | (frame_adrp >> 29) & 3
    if imm & (1 << 20): imm -= 1 << 21
    constants['game_frame_counter'] = (frame_at & ~4095) + (imm << 12) + ((frame_load >> 10) & 4095) * 4
    for at, key in [(0x2633e90, 'script_tls_thread'), (0x2633ec4, 'script_tls_active')]:
        word = struct.unpack('<I', main.read(at, 4))[0]
        if word & 0xffe0001f != 0xf2800000:
            raise ValueError('VM script TLS offset instruction changed')
        constants[key] = (word >> 5) & 0xffff
    constants['lock_mutex'] = thunks['_ZN2nn2os9LockMutexEPNS0_9MutexTypeE']['start']
    constants['unlock_mutex'] = thunks['_ZN2nn2os11UnlockMutexEPNS0_9MutexTypeE']['start']
    for label, symbol in FS.items():
        constants[label] = thunks[symbol]['start']
    for label, symbol in SDK.items():
        constants[label] = sdk_symbols[symbol]['value']
    loader_symbols = {
        'allocate_pages':'_ZN2nn2os14AllocateMemoryEPmm',
        'free_pages':'_ZN2nn2os15FreeMemoryPagesEmm',
        'ro_initialize':'_ZN2nn2ro10InitializeEv',
        'ro_register':'_ZN2nn2ro18RegisterModuleInfoEPNS0_16RegistrationInfoEPKv',
        'ro_unregister':'_ZN2nn2ro20UnregisterModuleInfoEPNS0_16RegistrationInfoE',
        'ro_load':'_ZN2nn2ro10LoadModuleEPNS0_6ModuleEPKvPvmi',
        'ro_unload':'_ZN2nn2ro12UnloadModuleEPNS0_6ModuleE',
        'ro_symbol':'_ZN2nn2ro18LookupModuleSymbolEPmPKNS0_6ModuleEPKc',
        'sha256':'_ZN2nn6crypto18GenerateSha256HashEPvmPKvm',
        'fs_size':'_ZN2nn2fs11GetFileSizeEPlNS0_10FileHandleE',
        'fs_open_dir':'_ZN2nn2fs13OpenDirectoryEPNS0_15DirectoryHandleEPKci',
        'fs_read_dir':'_ZN2nn2fs13ReadDirectoryEPlPNS0_14DirectoryEntryENS0_15DirectoryHandleEl',
        'fs_close_dir':'_ZN2nn2fs14CloseDirectoryENS0_15DirectoryHandleE',
    }
    for name, symbol in loader_symbols.items(): constants[name] = sdk_symbols[symbol]['value']
    # SDK Initialize is void and fatal on service failures; only call it when
    # the loader client has not already been created by the game.
    at=0x143ab8
    adrp,load=struct.unpack('<I',sdk.read(at,4))[0],struct.unpack('<I',sdk.read(0x143ac4,4))[0]
    if adrp&0x9f00001f!=0x90000008 or load&0xffc003ff!=(0xf9400000|(8<<5)):
        raise ValueError('SDK RO client instruction changed')
    imm=((adrp>>5)&0x7ffff)<<2|((adrp>>29)&3)
    if imm&(1<<20):imm-=1<<21
    constants['ro_client']=(at&~4095)+(imm<<12)+((load>>10)&4095)*8
    constants['free_address'] = 0x13a740
    anchor = '_ZN2nn2os16GetCurrentThreadEv'
    constants['sdk_anchor_slot'] = thunks[anchor]['slot']
    constants['sdk_anchor_offset'] = sdk_symbols[anchor]['value']
    anchor = '_ZN2nn2fs8MountRomEPKcPvm'
    constants['sdk_fallback_slot'] = thunks[anchor]['slot']
    constants['sdk_fallback_offset'] = sdk_symbols[anchor]['value']
    # Optional metadata is used by regression fixtures only. Runtime code uses
    # SD-native-bindings.bin and does not refer to these constants.
    custom = []
    menu_data = b''
    menu_code_start = b''
    if menu is not None:
        menu_data = Path(menu).read_bytes()
        constants['menu_name_hash'] = struct.unpack_from('<I', menu_data, 0x58)[0]
        constants['menu_code_bytes'] = struct.unpack_from('<I', menu_data, 0x1c)[0]
        constants['menu_native_count'] = struct.unpack_from('<I', menu_data, 0x2c)[0]
        table = struct.unpack_from('<Q', menu_data, 0x40)[0] & 0xffffff
        custom = []
        for index in range(constants['menu_native_count']):
            stored = struct.unpack_from('<Q', menu_data, table + index * 8)[0]
            rotate = (constants['menu_code_bytes'] + index) & 63
            decoded_hash = ((stored << rotate) | (stored >> ((64 - rotate) & 63))) & ((1 << 64) - 1)
            if decoded_hash in (0x4d45474153415645, 0x4d4547414c4f4144):
                custom.append(dict(index=index, hash=decoded_hash))
        if len(custom) != 2:
            raise ValueError('Paired menu must contain exactly MEGALOAD and MEGASAVE')
        blocks = struct.unpack_from('<Q', menu_data, 0x10)[0] & 0xffffff
        first_block = struct.unpack_from('<Q', menu_data, blocks)[0] & 0xffffff
        menu_code_start = menu_data[first_block:first_block + 32]
    for slot in (constants['vm_slot_base'], constants['vm_slot_derived']):
        relocations = [r for r in main.relocations if r['target'] == slot]
        if len(relocations) != 1 or relocations[0]['type'] != 1027 or relocations[0]['addend'] != constants['vm_tick']:
            raise ValueError('VM tick slot no longer matches the target relocation')
    fingerprints = {}
    for label, image, offsets in [
        ('main', main, [constants['vm_tick'], constants['native_register'], 0x2634444, 0x26332dc, frame_at]),
        ('sdk', sdk, [constants['mount_sd_debug'], constants['disable_thread_abort'], constants['restore_thread_context'],
                      constants['ro_register'],constants['ro_load'],constants['ro_symbol'],constants['allocate_pages'],
                      constants['ro_initialize'],constants['free_address'],constants['sha256']])
    ]:
        fingerprints[label] = [dict(offset=at, bytes=image.read(at, 16).hex()) for at in offsets]
    lines = ['#pragma once', '#include <stdint.h>', 'namespace binding {',
             'struct Fingerprint { uint32_t offset; uint8_t bytes[16]; };']
    for name, value in constants.items():
        lines.append(f'constexpr uint32_t {name} = {value:#x};')
    lines += ['struct MenuNative { uint32_t index; uint64_t hash; };',
              'constexpr MenuNative menu_natives[] = {']
    for entry in custom:
        lines.append('  {%d, %#xULL},' % (entry['index'], entry['hash']))
    lines += ['};', 'constexpr uint8_t menu_code_start[] = {' + ','.join(hex(v) for v in menu_code_start) + '};']
    for label, image in [('main', main), ('sdk', sdk)]:
        lines.append(f'constexpr uint64_t {label}_text_pages = {(image.layout[0]["size"] + 4095) & ~4095:#x};')
        lines.append(f'constexpr Fingerprint {label}_fingerprints[] = {{')
        for row in fingerprints[label]:
            lines.append('  {%#x, {%s}},' % (row['offset'], ','.join(hex(b) for b in bytes.fromhex(row['bytes']))))
        lines.append('};')
    lines.append('}')
    output.mkdir(parents=True, exist_ok=True)
    (output / 'bindings.hpp').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    report = dict(main_sha256=MAIN_SHA, main_build_id=main.data[0x40:0x60].hex(),
                  sdk_sha256=hashlib.sha256(sdk.data).hexdigest(), constants=constants,
                  fingerprints=fingerprints, vm_slot_proof='R_AARCH64_RELATIVE -> 0x2633e40',
                  menu_sha256=hashlib.sha256(menu_data).hexdigest() if menu_data else None, menu_fixture_only=True, menu_natives=custom,
                  menu_code_start=menu_code_start.hex(),
                  menu_cache_identity='name hash, code length, native count and 32-byte entry fingerprint',
                  native_context=dict(return_pointer=0, arg_count=8, args_pointer=16, data_count=24, script_slot_bytes=8,
                                      vector_destinations=32, vector_temporaries=64, vector_temporary_bytes=16,
                                      vector_plugin_bytes=24, caller_abi_version=1))
    (output / 'bindings.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    return report


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--target-profile', type=Path, default=DEFAULT_PROFILE)
    parser.add_argument('--target-main', type=Path); parser.add_argument('--target-sdk', type=Path)
    parser.add_argument('--menu', type=Path, help='Optional research-only menu fixture; requires both target images')
    args = parser.parse_args()
    if args.output.exists(): parser.error('Use a fresh output directory')
    try:
        result = generate(args.output.resolve(), args.menu, args.target_main, args.target_sdk, args.target_profile)
    except (OSError, ValueError) as error:
        parser.exit(1, str(error)+'\n')
    print('Bindings ready for target '+result['main_build_id'][:40])
