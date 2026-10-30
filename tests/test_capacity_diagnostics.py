"""Verify diagnostic report rejection and metric scope using synthetic reports."""
import bootstrap
import unittest
from capacity_diagnostics import summarize, PHASES, SAMPLE_FIELDS, GAPS


def fixture(**changes):
    values = dict(schema=2, run=2, phase='stopped', operation_error=0, startup_complete=1,
        clock_hz=19200000, clock_errors=0, session_start_tick=1, run_start_tick=2, elapsed_wall_us=1000000,
        workers_required=36, workers_admitted=36, workers_progressing=36, render_frames=2,
        image_pages_verified=513, bss_pages_verified=513, fragmentation_cycles=3, minimum_worker_ticks=1,
        memory_stats_available=1, memory_failures=0, release_failures=0,
        callback_intervals=1, callback_interval_total_us=33333, callback_interval_worst_us=33333,
        run_timing_available=1, run_completed_script_calls=100, run_script_total_us=200,
        completed_script_calls=200, script_total_us=400, script_worst_us=120, script_baseline_worst_us=120,
        report_failures=0, cleanup_order_errors=0, live_allocations=0, charged_bytes=0, requested_bytes=0,
        resource_count=0, cleanup_reclaimed=1, trim_succeeded=1, stop_callbacks=1, sessions_destroyed=1,
        children_created=2, children_destroyed=2, children_before_parent=2)
    values.update(dict.fromkeys(GAPS, 0)); values['callback_le_34ms'] = 1; values.update(changes)
    lines = ['ScriptHookV-NX capacity acceptance probe', *(f'{k}={v}' for k,v in values.items())]
    for phase in PHASES:
        sample = dict.fromkeys(SAMPLE_FIELDS, 0)
        sample.update(sample_count=1, sample_total_us=100, sample_worst_us=100,
                      sample_worst_at_wall_us=100, le_100us=1)
        lines += ['sample_phase='+phase, *(f'{k}={v}' for k,v in sample.items())]
    return ('\n'.join(lines)+'\n').encode()


class CapacityDiagnosticsTests(unittest.TestCase):
    def test_valid_report_does_not_claim_hardware_or_performance_acceptance(self):
        result = summarize(fixture())
        self.assertTrue(result['workload_reported_complete'])
        self.assertTrue(result['final_cleanup_reported_complete'])
        self.assertFalse(result['performance_acceptance_complete'])
        self.assertIn('identity', result['scope'])

    def test_running_workload_does_not_claim_final_cleanup(self):
        self.assertFalse(summarize(fixture(phase='running'))['final_cleanup_reported_complete'])

    def test_incomplete_failed_or_unmeasured_workload_is_distinguished(self):
        for change in (dict(operation_error=20), dict(workers_progressing=35), dict(startup_complete=0),
                       dict(image_pages_verified=0), dict(clock_errors=1), dict(fragmentation_cycles=0)):
            with self.subTest(change=change):self.assertFalse(summarize(fixture(**change))['workload_reported_complete'])

    def test_cleanup_requires_verified_zero_ownership_and_valid_memory_statistics(self):
        for change in (dict(live_allocations=1), dict(resource_count=1), dict(trim_succeeded=0),
                       dict(report_failures=1), dict(children_before_parent=1), dict(memory_stats_available=0)):
            with self.subTest(change=change):self.assertFalse(summarize(fixture(**change))['final_cleanup_reported_complete'])

    def test_duplicate_global_fields_are_rejected(self):
        with self.assertRaises(ValueError):summarize(fixture()+b'run=3\n')

    def test_duplicate_phase_is_rejected(self):
        with self.assertRaises(ValueError):summarize(fixture()+b'sample_phase=setup\n')

    def test_truncated_invalid_and_oversized_input_is_rejected(self):
        for raw in (fixture()[:-1], fixture()+b'\0\n', b'x'*8193, b'\xff\n', b'wrong header\n'):
            with self.subTest(raw=raw[:10]), self.assertRaises((ValueError, UnicodeError)):summarize(raw)

    def test_missing_phase_or_measurement_is_rejected(self):
        for raw in (fixture().replace(b'sample_phase=trim\n', b''),
                    fixture().replace(b'sample_count=1\n', b'', 1)):
            with self.assertRaises(ValueError):summarize(raw)

    def test_inconsistent_histograms_and_duration_are_rejected(self):
        for old,new in ((b'le_100us=1', b'le_100us=2'), (b'sample_total_us=100', b'sample_total_us=99'),
                        (b'sample_over_2ms=0', b'sample_over_2ms=1'),
                        (b'sample_worst_at_wall_us=100', b'sample_worst_at_wall_us=1000001')):
            with self.subTest(old=old), self.assertRaises(ValueError):summarize(fixture().replace(old,new,1))

    def test_callback_and_timing_totals_are_checked(self):
        for change in (dict(callback_intervals=2), dict(callback_interval_total_us=1),
                       dict(run_completed_script_calls=201), dict(run_script_total_us=401),
                       dict(session_start_tick=3)):
            with self.subTest(change=change), self.assertRaises(ValueError):summarize(fixture(**change))

    def test_old_schema_and_integer_overflow_are_rejected(self):
        for change in (dict(schema=1), dict(clock_hz=0), dict(run=0), dict(run=1 << 64), dict(run=-1)):
            with self.subTest(change=change), self.assertRaises(ValueError):summarize(fixture(**change))


if __name__ == '__main__':
    unittest.main()
