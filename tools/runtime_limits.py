"""Shared, portable build-default and optional-config helpers. No game files."""
from pathlib import Path
import re

HEADER = Path(__file__).resolve().parents[1]/'include/runtime_defaults.h'
KEYS = {
    'resident_limit': ('SHVNX_DEFAULT_RESIDENT_PLUGINS', 'SHVNX_MAX_RESIDENT_PLUGINS'),
    'script_slots': ('SHVNX_DEFAULT_SCRIPT_SLOTS', 'SHVNX_MAX_SCRIPT_SLOTS'),
    'plugin_memory_mib': ('SHVNX_DEFAULT_PLUGIN_MEMORY_MIB', 'SHVNX_MAX_PLUGIN_MEMORY_MIB'),
}

def definitions(header=HEADER):
    text = header.read_text(encoding='utf-8')
    values = {}
    for macro in [m for pair in KEYS.values() for m in pair]:
        matches = re.findall(r'^#define\s+'+macro+r'\s+([0-9]+)\s*$', text, re.M)
        if len(matches) != 1:
            raise ValueError('Expected one decimal definition for '+macro)
        values[macro] = int(matches[0])
    for macro, bound in zip((pair[1] for pair in KEYS.values()), (16, 64, 256)):
        if not 1 <= values[macro] <= bound:
            raise ValueError(f'{macro} must be between 1 and {bound}')
    for key, (default, maximum) in KEYS.items():
        if not 1 <= values[default] <= values[maximum]:
            raise ValueError('Compiled default outside ceiling: '+key)
    return values

def validate(settings, values=None):
    values = definitions() if values is None else values
    if not isinstance(settings, dict):
        raise ValueError('runtime_limits must be an object')
    for key, number in settings.items():
        if key not in KEYS:
            raise ValueError('Unknown runtime limit: '+str(key))
        if type(number) is not int or not 1 <= number <= values[KEYS[key][1]]:
            raise ValueError(f'{key} must be a whole number from 1 to {values[KEYS[key][1]]}')
    return settings

def render(settings, values=None):
    validate(settings, values)
    return ('# ScriptHookV-NX: optional global limits; restart GTA after editing.\n'
            '# Missing keys use this runtime build\'s defaults.\n'
            + ''.join(f'{key}={settings[key]}\n' for key in KEYS if key in settings))

def build_defaults(header, overrides):
    values = definitions(header)
    validate(overrides, values)
    text = header.read_text(encoding='utf-8')
    for key, number in overrides.items():
        macro = KEYS[key][0]
        text = re.sub(r'(^#define\s+'+macro+r'\s+)[0-9]+', lambda m: m[1]+str(number), text, flags=re.M)
    header.write_text(text, encoding='utf-8')
    return definitions(header)
