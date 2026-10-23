"""Summarize a loader report without reading settings or changing the SD card."""
import argparse
import json
from pathlib import Path

def summarize(path):
    if path.stat().st_size > 1024*1024: raise ValueError('Report exceeds 1 MiB')
    lines = path.read_text(encoding='utf-8-sig').splitlines()
    if not lines or not lines[0].startswith('ScriptHookV-NX'): raise ValueError('Expected a ScriptHookV-NX loader report')
    report = dict(header=lines[0], global_values={}, plugins=[], notes=[])
    current = report['global_values']
    for line in lines[1:]:
        if '=' not in line: continue
        key, value = line.split('=', 1)
        if key == 'plugin':
            current = dict(filename=value); report['plugins'].append(current)
        else: current[key] = value
    for plugin in report['plugins']:
        try: error = int(plugin.get('error', '0'), 0)
        except ValueError: error = -1
        if error == 0x10015:
            try: limit = int(report['global_values'].get('resident_limit', '0'), 0)
            except ValueError: limit = 0
            report['notes'].append(plugin['filename']+': '+(str(limit)+' resident modules' if limit else 'resident module limit')+' already reached; disable one and restart GTA')
        elif error == 0x10017:
            report['notes'].append(plugin['filename']+': keep exactly one Auto/Manual/Disabled filename for this plugin')
        elif error:
            report['notes'].append(plugin['filename']+': '+plugin.get('error_name', plugin.get('error', 'unknown error')))
    if report['global_values'].get('units')=='microseconds':
        for kind in ('script','callback','cleanup','initialize'):
            samples=[]
            for plugin in report['plugins']:
                try:
                    calls=int(plugin.get(kind+'_calls','0'));total=int(plugin.get(kind+'_total_us','0'));worst=int(plugin.get(kind+'_worst_us','0'))
                except ValueError:continue
                if calls>0 and total>=worst>=0:samples.append((worst,plugin['filename'],total//calls))
            if samples:
                worst,name,average=max(samples)
                report['notes'].append(f'Largest {kind} invocation: {name}, {worst} us elapsed (lifetime average {average} us). Includes native work, I/O and OS preemption; not FPS or isolated CPU time.')
    report['notes'].append('A report is a snapshot. Acceptance also needs the installed file hashes and an actual gameplay test.')
    return report

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__); p.add_argument('report', type=Path); a = p.parse_args()
    try: print(json.dumps(summarize(a.report), indent=2))
    except (OSError, ValueError) as error: p.exit(1, str(error)+'\n')
