"""T260: alternating native-frame A/B runs with isolated saves and identical explicit settings.

Requires both executables instrumented with XCDE_NATIVE_INTERVALS. This does not enable
the detailed CPU profiler. Captures are generated only in build/ (never publish game images).
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import statistics
import subprocess
from performance_report import summarize, compare


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--ini', type=Path, required=True)
    parser.add_argument('--route', type=Path, default=Path('tests/routes/title_idle.txt'))
    parser.add_argument('--targets', type=Path, default=Path('targets'))
    parser.add_argument('--save', type=Path)
    parser.add_argument('--frames', type=int, default=1200)
    parser.add_argument('--warmup', type=int, default=700)
    parser.add_argument('--runs', type=int, default=3)
    parser.add_argument('--timeout', type=int, default=180)
    parser.add_argument('--present', action='store_true', help='Present every native frame into the hidden window (includes V-Sync costs)')
    parser.add_argument('--output', type=Path, default=Path('build/ab-native'))
    args = parser.parse_args()
    if args.runs < 1 or args.warmup < 0 or args.frames <= args.warmup:
        parser.error('Need positive runs and frames > warmup >= 0')
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)  # never replace a previous benchmark's evidence
    reference, candidate = args.reference.resolve(), args.candidate.resolve()
    ini, route = args.ini.resolve(), args.route.resolve()
    config = hashlib.sha256(ini.read_bytes()).hexdigest()
    results = {'method': 'alternating, warm-cache native frames; no detailed profiler',
               'present_every_frame': args.present, 'warmup_frames': args.warmup,
               'ini_sha256': config, 'route_sha256': hashlib.sha256(route.read_bytes()).hexdigest(),
               'executables': {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in (reference, candidate)}, 'runs': []}
    for repetition in range(args.runs):
        order = [('reference', reference), ('candidate', candidate)]
        if repetition % 2:
            order.reverse()
        for label, exe in order:
            name = f'{label}-{repetition+1}'
            save = output / (name + '-save')
            if args.save:
                shutil.copytree(args.save, save)
            else:
                save.mkdir()
            env = os.environ.copy()
            for key in ('XCDE_METRICS', 'LARECOMP_VK_PROF', 'XCDE_PRESENT_ALWAYS', 'XCDE_SHOTS', 'XCDE_DESCRIPTOR_CACHE'):
                env.pop(key, None)
            env.update(XCDE_INI=str(ini), LARECOMP_SAVE_DIR=str(save), LARECOMP_HIDDEN='1', LARECOMP_NO_AUDIO='1',
                       XCDE_DETERMINISTIC='1', XCDE_INPUT_REPLAY=str(route), XCDE_EXIT_AFTER_SHOTS='1',
                       XCDE_SHOTS=f'{args.frames}:{output / (name + ".png")}', XCDE_NATIVE_INTERVALS=str(output / (name + '.csv')))
            if args.present:
                env['XCDE_PRESENT_ALWAYS'] = '1'
            env['PATH'] = str(root/'build/runtime-deps') + ';' + str(root/'build') + ';C:/msys64/ucrt64/bin;' + env['PATH']
            print('Running', name, flush=True)
            with (output/(name+'.log')).open('w', encoding='utf-8') as log:
                process = subprocess.Popen([str(exe), str(args.targets.resolve())], cwd=root, env=env, stdout=log,
                                           stderr=subprocess.STDOUT, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
                try:
                    code = process.wait(timeout=args.timeout)
                except subprocess.TimeoutExpired:
                    process.terminate()
                    process.wait()
                    raise SystemExit(f'{name} timed out; inspect its log')
            if code:
                raise SystemExit(f'{name} exited with {code}; inspect its log')
            if hashlib.sha256(ini.read_bytes()).hexdigest() != config:
                raise SystemExit('Settings changed during benchmark; comparison rejected')
            result = summarize(output/(name+'.csv'), args.warmup, args.frames-2)
            result['variant'], result['repetition'] = label, repetition+1
            results['runs'].append(result)
            (output/'results.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
            print(name, f'{result["native_fps"]:.2f} native FPS; p95 {result["frame_ms"]["p95"]:.2f} ms', flush=True)
    reference_runs = [r for r in results['runs'] if r['variant']=='reference']
    candidate_runs = [r for r in results['runs'] if r['variant']=='candidate']
    results['paired_changes'] = [compare(c,b) for b,c in zip(reference_runs,candidate_runs)]
    results['median_fps_change_percent'] = statistics.median(r['native_fps_change_percent'] for r in results['paired_changes'])
    (output/'results.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    print('Median FPS change:', results['median_fps_change_percent'], flush=True)


if __name__ == '__main__':
    main()
