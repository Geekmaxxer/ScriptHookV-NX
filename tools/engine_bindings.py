"""Emit independently guarded engine API offsets from an audited target profile."""
import json

def generate_engine(output, profile):
    record=json.loads((profile/'engine-api.json').read_text())
    target=json.loads((profile/'profile.json').read_text())
    binding=json.loads((profile/'bindings.json').read_text())
    if record.get('format')!=1 or record['build_id']!=target['build_id']:
        raise ValueError('Engine API profile does not match the selected target')
    if record.get('main_sha256')!=binding.get('main_sha256'):
        raise ValueError('Engine API executable fingerprint differs from the native caller profile')
    names={'global_blocks','global_sizes','guid_pool','guid_vtable','create_guid','ped_pool','vehicle_pool','object_pool','pickup_pool','keyboard_initialize','keyboard_states',
           'texture_factory','texture_factory_vtable','texture_create','texture_vtable','texture_valid','texture_delete','draw_sprite','rectangles','rectangle_buffer'}
    if set(record['constants'])!=names or len(record['proofs'])<15:
        raise ValueError('Incomplete engine API instruction contracts')
    lines=['#pragma once','#include <stdint.h>','namespace ScriptHook { namespace engine_binding {']
    for name,value in record['constants'].items():
        if type(value)!=int or not 0<value<0x8000000:raise ValueError('Invalid engine offset: '+name)
        lines.append(f'constexpr uint32_t {name}=0x{value:x};')
    lines+=['struct Proof { uint32_t offset; uint8_t bytes[16]; };','constexpr Proof proofs[]={']
    for proof in record['proofs']:
        data=bytes.fromhex(proof['bytes']);at=proof['offset']
        if len(data)!=16 or not 0<=at<0x3000000 or at%4:raise ValueError('Invalid engine instruction guard')
        lines.append('{0x%x,{%s}},'%(at,','.join('0x%02x'%b for b in data)))
    lines+=['};','} }']
    sdk_lines=['constexpr Proof sdk_proofs[]={']
    for proof in record['sdk_proofs']:
        data=bytes.fromhex(proof['bytes']);at=proof['offset']
        if len(data)!=16 or not 0<=at<0x800000 or at%4:raise ValueError('Invalid keyboard SDK instruction guard')
        sdk_lines.append('{0x%x,{%s}},'%(at,','.join('0x%02x'%b for b in data)))
    sdk_lines+=['};']
    lines[-1:-1]=sdk_lines
    (output/'engine_bindings.hpp').write_text('\n'.join(lines)+'\n',encoding='utf-8')
