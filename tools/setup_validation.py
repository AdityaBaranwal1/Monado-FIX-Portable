"""T253: fetch a project-local validation SDK from official MSYS2 packages.

No game data, system installation or PATH changes. Requires Python 3.14 (zstd)
or the zstandard module. Licences/metadata remain inside build/deps.
"""
from pathlib import Path
import hashlib
import io
import json
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
BASE = 'https://repo.msys2.org/mingw/ucrt64/'
PACKAGES = [
    'sdl3-3.4.8-1', 'vulkan-headers-1~1.4.363.0-1',
    'vulkan-loader-1~1.4.363.0-1', 'vulkan-memory-allocator-3.5.0-1',
    'shaderc-2026.4-1', 'winpthreads-14.0.0.r98.g19f5121a2-1',
    'libwinpthread-14.0.0.r98.g19f5121a2-1',
    'zlib-1.3.2-2',
]


def main():
    destination = ROOT / 'build/deps'
    downloads = ROOT / 'build/downloads'
    destination.mkdir(parents=True, exist_ok=True)
    downloads.mkdir(parents=True, exist_ok=True)
    manifest = []
    for package in PACKAGES:
        name = f'mingw-w64-ucrt-x86_64-{package}-any.pkg.tar.zst'
        archive = downloads / name
        url = BASE + name
        if not archive.exists():
            print(f'Downloading {name}', flush=True)
            with urllib.request.urlopen(url, timeout=60) as response:
                data = response.read()
            archive.write_bytes(data)
        data = archive.read_bytes()
        manifest.append({'url': url, 'sha256': hashlib.sha256(data).hexdigest()})
        try:
            from compression import zstd
            raw = zstd.decompress(data)
        except ImportError:
            import zstandard
            raw = zstandard.ZstdDecompressor().decompress(data, max_output_size=512 * 1024 * 1024)
        with tarfile.open(fileobj=io.BytesIO(raw), mode='r:') as source:
            source.extractall(destination, filter='data')
    (destination / 'validation-sdk.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(f'Validation SDK: {destination / "ucrt64"}', flush=True)


if __name__ == '__main__':
    main()
