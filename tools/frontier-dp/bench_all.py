import sys, time
sys.path.insert(0, sys.argv[1])
from frontier_dp import parse_tree, build_expanded_graph, topo_sort, count_frontier_dp
PRESETS='Engine/resources/presets.txt'
lines=[l for l in open(PRESETS,encoding='utf-8').read().split('\n') if l.strip()]
t0=time.time(); peak_overall=0; rows=[]; total_counted=0
for idx in range(1,len(lines)):   # skip 0 = "custom" welcome tree
    name=lines[idx].split(':')[1]
    h,nodes=parse_tree(PRESETS,idx)
    meta,par,chi=build_expanded_graph(nodes)
    order=topo_sort(meta,par,chi)
    cap=len(meta)              # full budget = every point slot
    t1=time.time()
    totals,peak=count_frontier_dp(meta,par,chi,order,cap)
    dt=time.time()-t1
    peak_overall=max(peak_overall,peak)
    biggest=max(totals.values())
    total_counted+=sum(totals.values())
    rows.append((name,len(nodes),len(meta),peak,biggest,dt))
elapsed=time.time()-t0
rows.sort(key=lambda r:-r[4])
print(f"{'preset':30}{'nodes':>6}{'slots':>6}{'states':>8}{'max count (any budget)':>26}{'sec':>7}")
print("-"*84)
for r in rows[:12]:
    print(f"{r[0][:30]:30}{r[1]:6d}{r[2]:6d}{r[3]:8d}{r[4]:26,d}{r[5]:7.3f}")
print("  ... (%d presets total)" % len(rows))
print()
print(f"ALL {len(rows)} presets, ALL budgets: {elapsed:.2f} s total")
print(f"peak DP states across every tree: {peak_overall:,}")
print(f"largest single count encountered: {max(r[4] for r in rows):,}")
