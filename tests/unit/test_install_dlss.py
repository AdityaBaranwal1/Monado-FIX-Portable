"""Official production-DLL selection, without downloading/loading vendor code."""
from pathlib import Path
import importlib.util
import unittest

spec = importlib.util.spec_from_file_location('install_dlss', Path(__file__).resolve().parents[2] / 'tools/install_dlss.py')
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)


class DLSSInstallerTests(unittest.TestCase):
    def test_selects_matching_production_files(self):
        names = [f'sdk/bin/x64/{name}' for name in installer.REQUIRED]
        names += ['sdk/bin/x64/development/sl.interposer.dll', 'sdk/bin/arm64/sl.dlss.dll']
        selected = installer.production_members(names)
        self.assertEqual(set(selected), set(installer.REQUIRED))
        self.assertEqual(selected['sl.interposer.dll'], 'sdk/bin/x64/sl.interposer.dll')

    def test_missing_or_duplicate_runtime_is_rejected(self):
        names = [f'bin/x64/{name}' for name in installer.REQUIRED]
        for invalid in (names[:-1], names + ['other/bin/x64/sl.common.dll']):
            with self.assertRaises(ValueError):
                installer.production_members(invalid)

    def test_pinned_release_matches_vendored_headers(self):
        header = (Path(__file__).resolve().parents[2] / 'third_party/streamline/include/sl_version.h').read_text()
        for name, value in zip(('MAJOR', 'MINOR', 'PATCH'), installer.VERSION.split('.')):
            self.assertIn(f'#define SL_VERSION_{name} {value}', header)
        self.assertTrue(installer.URL.startswith('https://github.com/NVIDIA-RTX/Streamline/releases/download/'))
        self.assertEqual(len(installer.SHA256), 64)

    def test_invalid_archive_path_is_rejected(self):
        with self.assertRaises(ValueError):
            installer.production_members(['../bin/x64/sl.interposer.dll'])
