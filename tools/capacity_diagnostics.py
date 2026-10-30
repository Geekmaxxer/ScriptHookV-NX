"""Read a Test2 capacity report without modifying its source or certifying hardware."""
import argparse
import hashlib
import json
from pathlib import Path
import re

PHASES = ('setup', 'extents', 'admission', 'verify_block', 'allocate_block',
          'report_format', 'report', 'cleanup', 'trim')
BUCKETS = ('le_100us', 'le_500us', 'le_1ms', 'le_2ms', 'le_10ms', 'le_50ms', 'gt_50ms')
GAPS = ('callback_le_17ms', 'callback_le_34ms', 'callback_le_50ms',
        'callback_le_100ms', 'callback_le_250ms', 'callback_le_1s', 'callback_gt_1s')
SAMPLE_FIELDS = ('sample_count', 'sample_total_us', 'sample_worst_us',
                 'sample_worst_at_wall_us', 'sample_over_2ms', *BUCKETS)


def summarize(raw):
    if len(raw) > 8192 or not raw.endswith(b'\n') or b'\0' in raw:
        raise ValueError('Oversized, incomplete or invalid capacity report')
    lines = raw.decode('utf-8').splitlines()
    if not lines or lines[0] != 'ScriptHookV-NX capacity acceptance probe':
        raise ValueError('Not a capacity acceptance report')
    values, phases, current = {}, {}, None
    for line in lines[1:]:
        if not re.fullmatch(r'[a-z][a-z0-9_]*=[a-z0-9_]+', line):
            raise ValueError('Invalid report field')
        key, value = line.split('=', 1)
        if key == 'sample_phase':
            if value not in PHASES or value in phases:
                raise ValueError('Unknown or duplicate measurement phase')
            current = {}; phases[value] = current
            continue
        target = current if key in SAMPLE_FIELDS else values
        if target is None or key in target:
            raise ValueError('Missing phase or duplicate field: ' + key)
        if key != 'phase':
            if not value.isdecimal() or len(value) > 20 or int(value) > (1 << 64)-1:
                raise ValueError('Invalid unsigned report value: ' + key)
            value = int(value)
        target[key] = value
    required = ('schema', 'run', 'operation_error', 'startup_complete', 'clock_hz', 'clock_errors',
                'session_start_tick', 'run_start_tick', 'elapsed_wall_us', 'phase',
                'workers_required', 'workers_admitted', 'workers_progressing', 'render_frames',
                'image_pages_verified', 'bss_pages_verified', 'fragmentation_cycles', 'minimum_worker_ticks',
                'memory_stats_available', 'memory_failures', 'release_failures',
                'callback_intervals', 'callback_interval_total_us', 'callback_interval_worst_us',
                'run_timing_available', 'run_completed_script_calls', 'run_script_total_us',
                'completed_script_calls', 'script_total_us', 'script_worst_us', 'script_baseline_worst_us',
                'report_failures', 'cleanup_order_errors', 'live_allocations', 'charged_bytes',
                'requested_bytes', 'resource_count', 'cleanup_reclaimed', 'trim_succeeded',
                'stop_callbacks', 'sessions_destroyed', 'children_created', 'children_destroyed', 'children_before_parent', *GAPS)
    if any(key not in values for key in required):
        raise ValueError('Required capacity fields are missing')
    if values['schema'] != 2 or values['clock_hz'] != 19200000 or not values['run']:
        raise ValueError('Unsupported report schema, clock or run ID')
    if values['phase'] not in ('admitting', 'running', 'stopped', 'failed', 'cleanup_failed'):
        raise ValueError('Unknown execution phase')
    if values['session_start_tick'] > values['run_start_tick']:
        raise ValueError('Run starts before its process-local session')
    if set(phases) != set(PHASES):
        raise ValueError('Incomplete phase measurements')
    for name, sample in phases.items():
        if set(sample) != set(SAMPLE_FIELDS):
            raise ValueError('Incomplete measurement: ' + name)
        if (sum(sample[key] for key in BUCKETS) != sample['sample_count'] or
                sample['sample_over_2ms'] != sum(sample[key] for key in BUCKETS[4:]) or
                sample['sample_total_us'] < sample['sample_worst_us'] or
                sample['sample_worst_at_wall_us'] > values['elapsed_wall_us']):
            raise ValueError('Inconsistent measurement: ' + name)
        if not sample['sample_count'] and any(sample.values()):
            raise ValueError('Measurements without samples: ' + name)
    if (sum(values[key] for key in GAPS) != values['callback_intervals'] or
            values['callback_intervals'] > max(0, values['render_frames']-1) or
            values['callback_interval_total_us'] < values['callback_interval_worst_us']):
        raise ValueError('Inconsistent callback interval measurements')
    if (values['run_completed_script_calls'] > values['completed_script_calls'] or
            values['run_script_total_us'] > values['script_total_us']):
        raise ValueError('Per-run timing exceeds owner lifetime timing')
    cleanup_keys = ('live_allocations', 'charged_bytes', 'requested_bytes', 'resource_count',
                    'report_failures', 'cleanup_order_errors', 'operation_error', 'clock_errors',
                    'memory_failures', 'release_failures')
    cleaned = (values['phase'] == 'stopped' and all(values[key] == 0 for key in cleanup_keys) and
               all(values[key] == 1 for key in ('cleanup_reclaimed', 'trim_succeeded',
                                               'stop_callbacks', 'sessions_destroyed', 'memory_stats_available')) and
               values['children_created'] == values['children_destroyed'] == values['children_before_parent'] == 2)
    measured = {key: value for key, value in phases.items() if value['sample_count']}
    return dict(report_sha256=hashlib.sha256(raw).hexdigest(), report=values, measurements=phases,
                slowest_measured_phase=max(measured, key=lambda name: measured[name]['sample_worst_us']) if measured else None,
                workload_reported_complete=values['operation_error'] == 0 and values['startup_complete'] == 1 and
                    values['clock_errors'] == values['report_failures'] == values['memory_failures'] == values['release_failures'] == 0 and
                    values['image_pages_verified'] == values['bss_pages_verified'] == 513 and
                    values['fragmentation_cycles'] > 0 and values['minimum_worker_ticks'] > 0 and
                    values['workers_required'] == values['workers_admitted'] == values['workers_progressing'] == 36,
                final_cleanup_reported_complete=cleaned,
                scope='Report structure and self-reported metrics only; artifact identity and hardware origin require independent checks',
                performance_acceptance_complete=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report', type=Path)
    args = parser.parse_args()
    try:
        # Bound the read even when pointed at an unexpected file.
        with args.report.open('rb') as source:
            result = summarize(source.read(8193))
    except (OSError, UnicodeError, ValueError) as error:
        parser.exit(1, str(error)+'\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
