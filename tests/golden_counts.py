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

# Filtered counts. The filter is positional: one value per talent in the preset,
# 0 = unconstrained, >0 = must have that many points, -1 = must not have any.
# These lock in the must-have pruning added to visitTalentFiltered -- pruning must
# change only the runtime, never the result.
#
# (preset index, name, points, [(talent position, value)], expected count, slow?)
FILTER_CASES = [
    (2, 'druid_restoration', 20, [(5, 1)],                              40_931, False),
    (2, 'druid_restoration', 20, [(5, 1), (12, 1), (18, 1)],            14_222, False),
    (2, 'druid_restoration', 20, [(5, 1), (12, 1), (18, 1),
                                  (7, -1), (9, -1)],                     2_329, False),
    (2, 'druid_restoration', 25, [(5, 1), (12, 1), (18, 1)],         4_209_348, False),
    (2, 'druid_restoration', 25, [(5, 1), (12, 1), (18, 1),
                                  (7, -1), (9, -1)],                   746_260, False),
    (2, 'druid_restoration', 30, [(5, 1), (12, 1), (18, 1),
                                  (7, -1), (9, -1)],                 9_464_517, False),
]

# Talent positions used for the complement property below.
COMPLEMENT_POSITIONS = [0, 5, 12, 20, 30]


def talent_count(preset_index):
    """Number of talents in a preset, read from its header."""
    line = open(PRESETS, encoding='utf-8').read().split('\n')[preset_index]
    return int(line.split(';')[0].split(':')[6])


def build_filter(preset_index, pairs):
    values = ['0'] * talent_count(preset_index)
    for pos, val in pairs:
        values[pos] = str(val)
    return ':'.join(values)


def run_case(exe, idx, points, expected, filter_str=None):
    args = [exe,
            '--structure-file-path', PRESETS,
            '--structure-indices', str(idx),
            '--target-talent-count', str(points),
            '--count-only',
            # above any expected count, so the guard never truncates a golden case
            '--max-results', '10000000000']
    if filter_str:
        args += ['--filter', filter_str]
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
    if expected is None:
        # caller only wants the number (used by the complement property below)
        return True, f'{actual:,}', dt
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
    total = 0
    for idx, name, points, expected, slow in cases:
        ok, detail, dt = run_case(exe, idx, points, expected)
        status = 'ok  ' if ok else 'FAIL'
        print(f'  [{status}] {name:28} @{points:2d}  {detail:>18}  ({dt:.1f}s)')
        total += 1
        if not ok:
            failures += 1

    # ---- filtered counts: pruning must change runtime, never results ----
    fcases = [c for c in FILTER_CASES if not (quick and c[5])]
    if fcases:
        print()
        print('  filtered:')
        for idx, name, points, pairs, expected, slow in fcases:
            fstr = build_filter(idx, pairs)
            ok, detail, dt = run_case(exe, idx, points, expected, fstr)
            status = 'ok  ' if ok else 'FAIL'
            shape = ','.join(f'{p}{"+" if v > 0 else "-"}' for p, v in pairs)
            print(f'  [{status}] {name:18} @{points:2d} [{shape:<22}] {detail:>14}  ({dt:.1f}s)')
            total += 1
            if not ok:
                failures += 1

    # ---- property: every build either has talent X or does not ----
    # must-have(X) + must-not-have(X) == unfiltered, for any X. This catches an
    # over-aggressive prune without needing a known-good baseline binary.
    print()
    print('  complement property (must-have + must-not-have == unfiltered):')
    base_ok, base_detail, _ = run_case(exe, 2, 20, None)
    base = int(base_detail.replace(',', '')) if base_detail.replace(',', '').isdigit() else None
    for pos in COMPLEMENT_POSITIONS:
        inc_ok, inc_d, _ = run_case(exe, 2, 20, None, build_filter(2, [(pos, 1)]))
        exc_ok, exc_d, _ = run_case(exe, 2, 20, None, build_filter(2, [(pos, -1)]))
        try:
            inc = int(inc_d.replace(',', ''))
            exc = int(exc_d.replace(',', ''))
        except ValueError:
            print(f'  [FAIL] talent {pos}: could not read counts')
            failures += 1
            total += 1
            continue
        ok = base is not None and inc + exc == base
        total += 1
        if not ok:
            failures += 1
        print(f'  [{"ok  " if ok else "FAIL"}] talent {pos:>2}: '
              f'{inc:>10,} + {exc:>10,} = {inc + exc:>10,}  (expected {base:,})')

    print()
    if failures:
        print(f'{failures} of {total} FAILED')
        return 1
    print(f'all {total} passed')
    return 0


if __name__ == '__main__':
    sys.exit(main())
