"""T249/T253: elapsed-time route conversion, independent of game assets."""
from pathlib import Path
import importlib.util
import unittest

MODULE = Path(__file__).resolve().parents[2] / 'tools/timed_route.py'
spec = importlib.util.spec_from_file_location('timed_route', MODULE)
timed_route = importlib.util.module_from_spec(spec)
spec.loader.exec_module(timed_route)


class TimedRouteTests(unittest.TestCase):
    def test_events_follow_time_instead_of_target_fps(self):
        text = timed_route.convert('1 0 0 0 0 0\n31 1 0 0 0 0\n61 0 0 0 0 0\n', 30)
        self.assertIn('# xcde input v2-ms', text)
        self.assertEqual(text.splitlines()[1:], ['0 0 0 0 0 0', '1000 1 0 0 0 0', '2000 0 0 0 0 0'])

    def test_fractional_source_rate(self):
        self.assertIn('1000 1', timed_route.convert('145 1 0 0 0 0', 144))

    def test_reject_unsorted_or_invalid_input(self):
        for route, fps in [('2 0 0 0 0 0\n1 0 0 0 0 0', 30), ('1 0', 30), ('1 0 0 0 0 0', 0)]:
            with self.assertRaises(ValueError):
                timed_route.convert(route, fps)
