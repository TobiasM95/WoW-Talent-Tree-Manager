#!/usr/bin/env python3
"""Cross-check the frontier DP against the C++ engine on ingested trees.

Two independent implementations reading the same tree JSON: the DP counts without
enumerating, the engine counts by enumerating. Disagreement means one is wrong.

    python tools/frontier-dp/crosscheck_json.py [trees_dir] [solver] [budgets...]
"""
import glob, json, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(HERE)), 'services', 'ingest'))
sys.path.insert(0, os.path.join('services', 'ingest'))

from frontier_dp import load_tree_json, build_expanded_graph, topo_sort, count_frontier_dp
from ttm_ingest import ttm_format as F

LEVEL_CAP = 90  # must be identical on both sides or they count different trees


def main():
    trees_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join('data', 'generated', 'trees')
    solver = os.path.normpath(sys.argv[2] if len(sys.argv) > 2 else
                              os.path.join('build-msvc', 'Release', 'ttm-solver.exe'))
    budgets = [int(b) for b in sys.argv[3:]] or [6, 10, 14]

    paths = sorted(glob.glob(os.path.join(trees_dir, '*.json')))
    if not paths:
        print(f'no trees in {trees_dir}', file=sys.stderr)
        return 2
    if not os.path.exists(solver):
        print(f'solver not found: {solver}', file=sys.stderr)
        return 2

    trees = [json.load(open(p, encoding='utf-8')) for p in paths]
    print(f'{len(trees)} trees, budgets {budgets}, level cap {LEVEL_CAP}')

    # one structure file holding every tree, so the engine runs once per budget
    tmp = tempfile.NamedTemporaryFile('w', suffix='.txt', delete=False,
                                      encoding='utf-8', newline='\n')
    tmp.write(F.trees_to_structure_file(trees, level_cap=LEVEL_CAP))
    tmp.close()

    # DP side
    dp = {}
    for path, tree in zip(paths, trees):
        _, nodes = load_tree_json(path, level_cap=LEVEL_CAP)
        meta, par, chi = build_expanded_graph(nodes)
        order = topo_sort(meta, par, chi)
        totals, _ = count_frontier_dp(meta, par, chi, order, max(budgets))
        dp[tree['key']] = totals

    failures = 0
    checked = 0
    try:
        for budget in budgets:
            out = subprocess.run(
                [solver, '--structure-file-path', tmp.name,
                 '--target-talent-count', str(budget), '--count-only',
                 '--max-results', '10000000000'],
                capture_output=True, text=True, timeout=7200).stdout
            rows = re.findall(r'Tree (\d+): (\d+) combinations', out)
            if len(rows) != len(trees):
                print(f'  budget {budget}: engine reported {len(rows)} trees, '
                      f'expected {len(trees)} -- index mapping unreliable, aborting')
                return 1
            if 'INCOMPLETE' in out:
                print(f'  budget {budget}: engine hit its guard, results truncated')
                return 1
            bad = []
            for (_, count), tree in zip(rows, trees):
                expected = dp[tree['key']].get(budget, 0)
                checked += 1
                if int(count) != expected:
                    bad.append((tree['key'], int(count), expected))
            if bad:
                failures += len(bad)
                print(f'  budget {budget:2}: {len(bad)} MISMATCH of {len(trees)}')
                for key, got, want in bad[:5]:
                    print(f'      {key:28} engine {got:,} vs dp {want:,}')
            else:
                total = sum(dp[t['key']].get(budget, 0) for t in trees)
                print(f'  budget {budget:2}: all {len(trees)} agree   '
                      f'(total builds across all trees: {total:,})')
    finally:
        os.unlink(tmp.name)

    print()
    if failures:
        print(f'{failures} of {checked} comparisons FAILED')
        return 1
    print(f'all {checked} comparisons agree')
    return 0


if __name__ == '__main__':
    sys.exit(main())
