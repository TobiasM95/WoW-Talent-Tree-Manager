import subprocess, sys, re
sys.path.insert(0, sys.argv[1])
from frontier_dp import parse_tree, build_expanded_graph, topo_sort, count_frontier_dp

PRESETS='Engine/resources/presets.txt'
EXE='./build-msvc/Release/ttm-solver.exe'
names=[l.split(':')[1] for l in open(PRESETS,encoding='utf-8').read().split('\n') if l.strip()]

# pick a spread: class trees and spec trees across several classes
cases=[(1,15),(1,20),(2,20),(3,18),(4,20),(11,18),(20,18),(31,18),(45,18),(60,18),(77,18)]
print(f"{'idx':>3} {'preset':28} {'pts':>4} {'DP':>14} {'engine':>14}  match")
print("-"*82)
allok=True
for idx,pts in cases:
    if idx>=len(names): continue
    h,nodes=parse_tree(PRESETS,idx)
    meta,par,chi=build_expanded_graph(nodes)
    order=topo_sort(meta,par,chi)
    totals,_=count_frontier_dp(meta,par,chi,order,pts)
    dp=totals.get(pts,0)
    out=subprocess.run([EXE,'--structure-file-path',PRESETS,'--structure-indices',str(idx),
                        '--target-talent-count',str(pts),'--count-only'],
                       capture_output=True,text=True,timeout=1800).stdout
    m=re.search(r'Tree 0: (\d+) combinations',out)
    eng=int(m.group(1)) if m else -1
    ok = dp==eng
    allok = allok and ok
    print(f"{idx:3d} {names[idx][:28]:28} {pts:4d} {dp:14,d} {eng:14,d}  {'OK' if ok else 'MISMATCH'}")
print()
print("ALL MATCH" if allok else "*** MISMATCHES PRESENT ***")
