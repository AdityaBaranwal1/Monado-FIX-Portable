"""T260: summarize opt-in native-frame CSVs; compare equal-settings captures explicitly.

python tools/performance_report.py build/native_metrics.csv --start-frame 300
python tools/performance_report.py build/native_metrics.csv --compare build/reference_metrics.csv
No loading frames are silently removed. Select an identical gameplay range for A/B tests.
"""
import argparse
import csv
import hashlib
import io
import json
import math
from pathlib import Path


def percentile(values, percent):
    values = sorted(values)
    if not values:
        return None
    position = (len(values) - 1) * percent / 100
    low = math.floor(position)
    high = math.ceil(position)
    return values[low] + (values[high] - values[low]) * (position - low)


def summarize(path, start=0, end=None):
    metadata = {}
    lines = []
    for line in Path(path).read_text(encoding='utf-8').splitlines():
        if line.startswith('#'):
            key, _, value = line[1:].partition('=')
            metadata[key] = value
        else:
            lines.append(line)
    rows = list(csv.DictReader(io.StringIO('\n'.join(lines))))
    samples = []
    for row in rows:
        try:
            frame = int(row['frame'])
            duration = float(row['frame_ms'])
        except (KeyError, ValueError) as exc:
            raise ValueError('Invalid metrics row') from exc
        if duration > 0 and math.isfinite(duration) and frame >= start and (end is None or frame <= end):
            samples.append(row)
    if not samples:
        raise ValueError('No complete native frames in the selected range')
    times = [float(r['frame_ms']) for r in samples]
    gpu = [float(r['gpu_ms']) for r in samples if float(r['gpu_ms']) >= 0 and math.isfinite(float(r['gpu_ms']))]
    return {
        'source': str(path), 'sha256': hashlib.sha256(Path(path).read_bytes()).hexdigest(),
        'metadata': metadata, 'frames': len(times), 'native_fps': 1000 * len(times) / sum(times),
        'frame_ms': {f'p{p}': percentile(times, p) for p in (50, 95, 99)},
        'gpu_ms_p50': percentile(gpu, 50), 'stalls_over_250ms': sum(t > 250 for t in times),
        'descriptor_hits': sum(int(r.get('descriptor_hits', 0)) for r in samples),
        'descriptor_updates': sum(int(r.get('descriptor_updates', 0)) for r in samples),
        'gmem_upload_mib': sum(float(r.get('gmem_upload_mib', 0)) for r in samples),
        'gmem_readback_mib': sum(float(r.get('gmem_readback_mib', 0)) for r in samples),
    }


def compare(candidate, baseline):
    # A different profile is a quality comparison, not evidence of an internal optimization.
    if candidate['metadata'] != baseline['metadata']:
        raise ValueError('GPU, driver or settings differ: use equal settings for an internal A/B comparison')
    return {
        'native_fps_change_percent': 100 * (candidate['native_fps'] / baseline['native_fps'] - 1),
        'p95_change_percent': 100 * (candidate['frame_ms']['p95'] / baseline['frame_ms']['p95'] - 1),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv', type=Path)
    parser.add_argument('--compare', type=Path)
    parser.add_argument('--start-frame', type=int, default=0)
    parser.add_argument('--end-frame', type=int)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    try:
        result = summarize(args.csv, args.start_frame, args.end_frame)
        if args.compare:
            baseline = summarize(args.compare, args.start_frame, args.end_frame)
            result['comparison'] = compare(result, baseline)
            result['baseline'] = baseline
    except (ValueError, OSError) as exc:
        parser.error(str(exc))
    text = json.dumps(result, indent=2, ensure_ascii=False) + '\n'
    if args.output:
        args.output.write_text(text, encoding='utf-8')
    print(text, end='')


if __name__ == '__main__':
    main()
