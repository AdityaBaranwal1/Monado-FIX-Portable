"""T249/T253: convert a frame-based input route to elapsed milliseconds.

python tools/timed_route.py tests/routes/load_walk.txt build/load_walk_ms.txt --source-fps 30
Runtime detects the v2-ms header; events no longer depend on achieved render FPS.
"""
from pathlib import Path
import argparse


def convert(text, fps):
    if fps <= 0:
        raise ValueError('source FPS must be positive')
    result = ['# xcde input v2-ms: <elapsed milliseconds> <buttons hex> <lx> <ly> <rx> <ry>']
    previous = -1
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        fields = line.split()
        if len(fields) != 6:
            raise ValueError(f'expected six fields: {line}')
        frame = int(fields[0])
        if frame < previous or frame < 0:
            raise ValueError('events must be sorted and nonnegative')
        previous = frame
        fields[0] = str(round(max(0, frame - 1) * 1000 / fps))
        result.append(' '.join(fields))
    return '\n'.join(result) + '\n'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--source-fps', type=float, required=True)
    args = parser.parse_args()
    args.destination.write_text(convert(args.source.read_text(encoding='utf-8'), args.source_fps), encoding='utf-8')
