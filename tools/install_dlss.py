"""T257: install matching production DLSS SR binaries from NVIDIA's official release.

Downloads the complete official SDK, verifies its pinned SHA-256, extracts only
bin/x64 production runtime DLLs plus licence files, then atomically deploys them.
No driver changes, signature bypasses or third-party DLL download sites.
"""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import os
import shutil
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = '2.14.16'
URL = f'https://github.com/NVIDIA-RTX/Streamline/releases/download/v{VERSION}/streamline-sdk-v{VERSION}.zip'
SHA256 = '32041e51db0cab709a271b72a352281b690c2abbd669a827809401a5dd23956b'
REQUIRED = ('sl.interposer.dll', 'sl.common.dll', 'sl.dlss.dll', 'nvngx_dlss.dll')
OPTIONAL = ('NvLowLatencyVk.dll',)


def production_members(names):
    selected = {}
    for name in names:
        p = PurePosixPath(name.replace('\\', '/'))
        if '..' in p.parts or p.is_absolute():
            raise ValueError('Invalid archive path')
        if p.name not in REQUIRED + OPTIONAL:
            continue
        # Excludes development/debug and non-x64 SDK runtimes.
        if p.parts[-3:-1] != ('bin', 'x64'):
            continue
        if p.name in selected:
            raise ValueError(f'Ambiguous production DLL: {p.name}')
        selected[p.name] = name
    missing = set(REQUIRED) - selected.keys()
    if missing:
        raise ValueError(f'Missing official production DLLs: {sorted(missing)}')
    return selected


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('--destination', type=Path, default=ROOT / 'build')
    args = parser.parse_args()
    downloads = ROOT / 'build/downloads'
    downloads.mkdir(parents=True, exist_ok=True)
    archive = downloads / f'streamline-sdk-v{VERSION}.zip'
    if not archive.is_file():
        temporary = archive.with_suffix('.zip.part')
        print(f'Downloading official NVIDIA Streamline {VERSION} (276 MB)', flush=True)
        request = urllib.request.Request(URL, headers={'User-Agent': 'Monado-DLSS-Installer'})
        with urllib.request.urlopen(request, timeout=60) as response, temporary.open('wb') as out:
            shutil.copyfileobj(response, out, 1024 * 1024)
        os.replace(temporary, archive)
    with archive.open('rb') as source:
        digest = hashlib.file_digest(source, 'sha256').hexdigest()
    if digest != SHA256:
        raise ValueError('Official SDK SHA-256 mismatch; no DLLs installed')
    sdk = ROOT / 'sdk/sl'
    runtime = sdk / 'bin/x64'
    runtime.mkdir(parents=True, exist_ok=True)
    manifest = {'version': VERSION, 'source': URL, 'sdk_sha256': digest, 'files': {}}
    with zipfile.ZipFile(archive) as z:
        members = production_members(z.namelist())
        for name, member in members.items():
            data = z.read(member)
            if not data.startswith(b'MZ'):
                raise ValueError(f'{name}: not a Windows DLL')
            (runtime / name).write_bytes(data)
            manifest['files'][name] = hashlib.sha256(data).hexdigest()
        for member in z.namelist():
            base = PurePosixPath(member).name
            if not member.endswith('/') and ('license' in base.lower() or 'eula' in base.lower()) and z.getinfo(member).file_size < 2 * 1024 * 1024:
                license_dir = sdk / 'licenses'
                license_dir.mkdir(exist_ok=True)
                (license_dir / base).write_bytes(z.read(member))
    args.destination.mkdir(parents=True, exist_ok=True)
    # Check normal file locks before staging; do not update an actively used runtime.
    for name in manifest['files']:
        target = args.destination / name
        if target.exists():
            with target.open('r+b'):
                pass
    staged = []
    for name in manifest['files']:
        target = args.destination / name
        temporary = target.with_name(name + '.install.tmp')
        shutil.copyfile(runtime / name, temporary)
        staged.append((temporary, target))
    for temporary, target in staged:
        os.replace(temporary, target)
    if (sdk / 'licenses').is_dir():
        shutil.copytree(sdk / 'licenses', args.destination / 'dlss-licenses', dirs_exist_ok=True)
    (args.destination / 'dlss-runtime.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(f'Installed DLSS SR / Streamline {VERSION}: {args.destination.resolve()}', flush=True)


if __name__ == '__main__':
    main()
