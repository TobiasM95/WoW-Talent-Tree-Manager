#!/usr/bin/env python3
"""
Golden-count regression test for the solver.

Asserts that ttm-solver returns exact, known-correct combination counts for a set
of real presets and budgets. Every count here was independently confirmed two ways:
by the C++ enumerator and by the frontier DP in tools/frontier-dp/.

Why this exists: the engine's hot path was modified five times in one session and
each regression was found by chance rather than by a test. Specifically caught by
these cases:

  - a 32-bit runningCount overflowing at 2^31 (shaman_class_elemental @24)
  - a caller's default safetyGuard clamping every solve to 500,000,000 (@30)
  - --count-only not reaching the default, non-parallel dispatch branch
  - a null filter segfaulting when --filter was omitted (every case)
  - CRLF data files failing format validation on Linux (every case)

Usage:
    python tests/golden_counts.py [path/to/ttm-solver] [--quick]

--quick skips cases marked slow (the >2^31 overflow case takes ~8 minutes).
Exit code 0 on success, 1 on any mismatch.
"""
import subprocess
import sys
import re
import time
import os

PRESETS = os.path.join('Engine', 'resources', 'presets.txt')

# (preset index, preset name, talent points, expected count, slow?)
CASES = [
    (2,  'druid_restoration',            10,             223, False),
    (2,  'druid_restoration',            15,          17_541, False),
    (2,  'druid_restoration',            20,          71_030, False),
    (1,  'druid_class_restoration',      15,         668_097, False),
    (1,  'druid_class_restoration',      20,       3_595_878, False),
    (3,  'druid_class_feral',            18,       1_598_023, False),
    (4,  'druid_feral',                  20,          27_971, False),
    (11, 'evoker_class_preservation',    18,         507_168, False),
    (20, 'priest_discipline',            18,          95_829, False),
    (31, 'hunter_class_survival',        18,       1_310_774, False),
    (45, 'paladin_class_protection',     18,       1_831_683, False),
    (60, 'warrior_protection',           18,          61_371, False),
    (77, 'demonhunter_class_vengeance',  18,         674_398, False),
    # guards against the default-safetyGuard clamp (would report exactly 500,000,000)
    (2,  'druid_restoration',            30,     305_286_987, True),
    # guards against 32-bit counter overflow: exceeds INT_MAX (2,147,483,647)
    (43, 'shaman_class_elemental',       24,   3_325_013_320, True),
]

COUNT_RE = re.compile(r'Tree 0: (\d+) combinations')


def run_case(exe, idx, points, expected):
    args = [exe,
            '--structure-file-path', PRESETS,
            '--structure-indices', str(idx),
            '--target-talent-count', str(points),
            '--count-only',
            # above any expected count, so the guard never truncates a golden case
            '--max-results', '10000000000']
    t0 = time.time()
    proc = subprocess.run(args, capture_output=True, text=True, timeout=3600)
    dt = time.time() - t0
    out = proc.stdout

    if proc.returncode != 0:
        return False, f'exit code {proc.returncode}', dt
    if 'INCOMPLETE' in out:
        return False, 'safety guard triggered (result truncated)', dt
    m = COUNT_RE.search(out)
    if not m:
        return False, f'no count in output: {out.strip()[:120]!r}', dt
    actual = int(m.group(1))
    if actual != expected:
        return False, f'got {actual:,}, expected {expected:,}', dt
    return True, f'{actual:,}', dt


def main():
    args = [a for a in sys.argv[1:] if a != '--quick']
    quick = '--quick' in sys.argv[1:]
    exe = args[0] if args else os.path.join('build', 'ttm-solver')
    if not os.path.exists(exe) and os.path.exists(exe + '.exe'):
        exe += '.exe'
    # Windows CreateProcess rejects a RELATIVE path written with forward slashes,
    # reporting "file not found" even though the file is plainly there.
    exe = os.path.normpath(exe)
    if not os.path.exists(exe):
        print(f'solver not found: {exe}', file=sys.stderr)
        print('build it first:  cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j',
              file=sys.stderr)
        return 2
    if not os.path.exists(PRESETS):
        print(f'presets not found: {PRESETS} (run from the repository root)', file=sys.stderr)
        return 2

    cases = [c for c in CASES if not (quick and c[4])]
    print(f'solver: {exe}')
    print(f'{len(cases)} cases' + (' (quick: slow cases skipped)' if quick else ''))
    print()

    failures = 0
    for idx, name, points, expected, slow in cases:
        ok, detail, dt = run_case(exe, idx, points, expected)
        status = 'ok  ' if ok else 'FAIL'
        print(f'  [{status}] {name:28} @{points:2d}  {detail:>18}  ({dt:.1f}s)')
        if not ok:
            failures += 1

    print()
    if failures:
        print(f'{failures} of {len(cases)} FAILED')
        return 1
    print(f'all {len(cases)} passed')
    return 0


if __name__ == '__main__':
    sys.exit(main())
