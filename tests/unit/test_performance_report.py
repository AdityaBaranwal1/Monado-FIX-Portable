import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('performance_report', Path(__file__).resolve().parents[2] / 'tools/performance_report.py')
report = importlib.util.module_from_spec(spec)
spec.loader.exec_module(report)


class PerformanceReportTests(unittest.TestCase):
    def test_native_rate_is_time_weighted_and_long_stalls_are_retained(self):
        with tempfile.TemporaryDirectory() as folder:
            file = Path(folder) / 'capture.csv'
            file.write_text('#gpu=fixture\nframe,frame_ms,gpu_ms\n0,0,-1\n1,10,4\n2,20,8\n3,300,-1\n', encoding='utf-8')
            result = report.summarize(file)
            self.assertEqual(result['frames'], 3)
            self.assertAlmostEqual(result['native_fps'], 3000 / 330)
            self.assertEqual(result['stalls_over_250ms'], 1)
            self.assertEqual(result['gpu_ms_p50'], 6)
            self.assertEqual(report.summarize(file, 1, 2)['frame_ms']['p50'], 15)

    def test_empty_capture_and_different_quality_are_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            file = Path(folder) / 'capture.csv'
            file.write_text('frame,frame_ms,gpu_ms\n0,0,-1\n', encoding='utf-8')
            with self.assertRaises(ValueError):
                report.summarize(file)
        with self.assertRaises(ValueError):
            report.compare({'metadata': {'preset': 'potato'}}, {'metadata': {'preset': 'balanced'}})

    def test_equal_configuration_comparison(self):
        base = {'metadata': {'gpu': 'fixture'}, 'native_fps': 20, 'frame_ms': {'p95': 50}}
        candidate = {'metadata': {'gpu': 'fixture'}, 'native_fps': 30, 'frame_ms': {'p95': 35}}
        diff = report.compare(candidate, base)
        self.assertAlmostEqual(diff['native_fps_change_percent'], 50)
        self.assertAlmostEqual(diff['p95_change_percent'], -30)


if __name__ == '__main__':
    unittest.main()
