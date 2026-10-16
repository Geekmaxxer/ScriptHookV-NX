#!/usr/bin/env python3
"""Generate Switch wrappers without guessing unknown argument/reference types."""
import argparse
import hashlib
import json
import re
from pathlib import Path

TYPE_MAP = {
    'BOOL': 'bool', 'INT': 'int32_t', 'FLOAT': 'float', 'STRING': 'const char*',
    'VECTOR': 'Vector3', 'VECTOR3': 'Vector3', 'PED_INDEX': 'Ped',
    'VEHICLE_INDEX': 'Vehicle', 'ENTITY_INDEX': 'Entity', 'PLAYER_INDEX': 'Player',
    'OBJECT_INDEX': 'Object', 'CAMERA_INDEX': 'Cam', 'BLIP_INDEX': 'Blip',
    'PICKUP_INDEX': 'Pickup', 'WEAPON_TYPE': 'Hash', 'MODEL_NAMES': 'Hash',
    'HASH': 'Hash', 'SCR_HASH': 'Hash', 'VOID': 'void', 'ANY': 'Any',
    'ENUM_TO_INT': 'int32_t',
    'NAVDATA': 'NativeNavData',
}
KEYWORDS = set(('alignas alignof and and_eq asm atomic_cancel atomic_commit atomic_noexcept auto bitand bitor '
    'bool break case catch char char8_t char16_t char32_t class compl concept const consteval constexpr constinit '
    'const_cast continue co_await co_return co_yield decltype default delete do double dynamic_cast else enum '
    'explicit export extern false float for friend goto if inline int long mutable namespace new noexcept not '
    'not_eq nullptr operator or or_eq private protected public register reinterpret_cast requires return short '
    'signed sizeof static static_assert static_cast struct switch synchronized template this thread_local throw '
    'true try typedef typeid typename union unsigned using virtual void volatile wchar_t while xor xor_eq').split())
PATTERN = re.compile(r'NATIVE\s+(?:FUNC\s+(\w+)|PROC)\s+(\w+)\s*\((.*?)\)\s*=\s*"(0x[0-9a-fA-F]+)"', re.S)

def strip_comments(text):
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)

def split_arguments(text):
    parts, start, depth = [], 0, 0
    for index, char in enumerate(text):
        if char in '([': depth += 1
        elif char in ')]': depth -= 1
        elif char == ',' and depth == 0:
            parts.append(text[start:index]); start = index + 1
    if text[start:].strip(): parts.append(text[start:])
    return parts

def declarations(core):
    enums, structures, opaque, sources = set(), set(), set(), {}
    paths = sorted(core.rglob('*.sch'))
    router = core.parent / 'multiplayer/scripts/Transition/Transition_Script_Router_Defs.sch'
    if router.is_file(): paths.append(router)
    for path in paths:
        data = path.read_bytes()
        text = strip_comments(data.decode('utf-8', errors='replace'))
        found = False
        for kind, destination in [('ENUM', enums), ('STRUCT', structures)]:
            prefix = r'(?:(?:HASH|STRICT)_)' if kind == 'ENUM' else ''
            names = re.findall(r'^\s*(?:' + prefix + r')?' + kind + r'\s+(\w+)\b', text, re.M | re.I)
            destination.update(name.upper() for name in names); found |= bool(names)
        names = re.findall(r'^\s*NATIVE\s+(\w+)\s*(?::\s*\w+\s*)?$', text, re.M)
        opaque.update(name.upper() for name in names); found |= bool(names)
        if found: sources[path.relative_to(core.parent).as_posix()] = hashlib.sha256(data).hexdigest()
    return enums, structures, opaque, sources

def cpp_type(raw, reference, enums, structures, opaque):
    kind = raw.upper()
    if kind == 'NAVDATA' and not reference: return 'NativeNavData'
    if kind == 'STRUCT' or kind in structures:
        if not reference: raise ValueError('structure passed without a reference: ' + raw)
        return 'void*'
    if kind.startswith('TEXT_LABEL'):
        return 'char*' if reference else 'const char*'
    if kind == 'BOOL': return 'BOOL*' if reference else 'bool'
    if kind in TYPE_MAP:
        mapped = TYPE_MAP[kind]
        return mapped + '*' if reference else mapped
    if kind in enums: return 'int32_t*' if reference else 'int32_t'
    if kind in opaque:
        # Preserve all bits of opaque native values. Their referenced storage
        # is deliberately caller-defined, not a guessed int or struct layout.
        return 'void*' if reference else 'Any'
    raise ValueError('unknown native type: ' + raw)

def parse_arg(text, index, enums, structures, opaque):
    text = text.split('=', 1)[0].strip()
    match = re.fullmatch(r'(\w+)\s*(&)?\s*(\w+)?\s*(\[[^\]]*\])?', text)
    if not match: raise ValueError('unrecognized parameter: ' + text)
    raw, ref, name, array = match.groups()
    name = name or ('arg_' + str(index))
    if name in KEYWORDS: name += '_'
    mapped = cpp_type(raw, bool(ref or array), enums, structures, opaque)
    return dict(type=mapped, name=name, source_type=raw, reference=bool(ref or array))

def generate(dev_root, output, report, catalog):
    core = dev_root / 'core'
    enums, structures, opaque, type_sources = declarations(core)
    registered = {int(row['hash'], 16) for row in json.loads(catalog.read_text())['entries']} if catalog else set()
    modules, skipped, sources, signatures = {}, [], {}, []
    for path in sorted((core / 'common/native').glob('commands_*.sch')):
        data = path.read_bytes(); sources[path.name] = hashlib.sha256(data).hexdigest()
        declarations_text = strip_comments(data.decode('utf-8', errors='replace'))
        module = path.stem.removeprefix('commands_').upper()
        names = set()
        for match in PATTERN.finditer(declarations_text):
            ret, name, args, hash_text = match.groups()
            if name in names: raise ValueError('duplicate wrapper: ' + module + '::' + name)
            names.add(name)
            try:
                result = cpp_type(ret or 'VOID', False, enums, structures, opaque)
                parameters = [parse_arg(arg, i, enums, structures, opaque) for i, arg in enumerate(split_arguments(args))]
            except ValueError as error:
                skipped.append(dict(module=module, name=name, hash=hash_text, reason=str(error))); continue
            row = dict(module=module, name=name, return_type=result, arguments=parameters, hash=hash_text,
                       registered_in_target=int(hash_text, 16) in registered if catalog else None)
            modules.setdefault(module, []).append(row); signatures.append(row)
    if not signatures: raise ValueError('No wrappers generated')
    lines = ['// Generated from dev_ng declarations; runtime availability is build-specific.',
             '// References and opaque data require storage matching the native contract.',
             '#pragma once', '#include "types.h"', '#include "nativeCaller.h"', '',
             'namespace ScriptHook { namespace Natives {']
    for module, rows in modules.items():
        lines.append('namespace ' + module + ' {')
        for row in rows:
            parameters = ', '.join(p['type'] + ' ' + p['name'] for p in row['arguments'])
            call = 'ScriptHook::invoke<' + row['return_type'] + '>(' + row['hash'] + 'ULL'
            call += ''.join(', ' + p['name'] for p in row['arguments']) + ')'
            lines += ['inline ' + row['return_type'] + ' ' + row['name'] + '(' + parameters + ') {',
                      '    ' + ('return ' if row['return_type'] != 'void' else '') + call + ';', '}']
        lines.append('}')
    lines += ['} }', '']
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines), encoding='utf-8')
    proof = dict(wrapper_count=len(signatures), namespaces=len(modules),
                 target_registered_wrappers=sum(row['registered_in_target'] is True for row in signatures),
                 unknown_types_are_not_guessed=True, bool_references_use_32_bit_BOOL=True,
                 opaque_references_use_caller_defined_storage=True, skipped=skipped,
                 header_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),
                 source_headers_sha256=sources, source_type_headers_sha256=type_sources, signatures=signatures,
                 runtime_verified=False)
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(proof, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k:proof[k] for k in ['wrapper_count','namespaces','target_registered_wrappers']}) +
          ' skipped=' + str(len(skipped)))
    return proof

if __name__ == '__main__':
    project = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dev-root', type=Path, required=True, help='Your local compiler/header tree; not bundled')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--catalog', type=Path, help='Optional audited static registration catalog; not guessed when omitted')
    args = parser.parse_args()
    generate(args.dev_root, args.output, args.report, args.catalog)
