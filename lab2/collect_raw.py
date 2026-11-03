#!/usr/bin/env python3
import os, shutil, statistics, itertools, datetime
BASE, OUT = "campaign", "raw"
TRIALS = sorted(t for t in os.listdir(BASE) if os.path.isdir(os.path.join(BASE, t)))
def stats(t):
    d = {}
    for line in open(os.path.join(BASE, t, "default", "fuzzer_stats")):
        if ":" in line:
            k, v = line.split(":", 1); d[k.strip()] = v.strip()
    return d
def edges_at(t, secs):
    best = None
    with open(os.path.join(BASE, t, "default", "plot_data")) as f:
        hdr = f.readline().lstrip("# ").strip().split(", ")
        ti, ei = hdr.index("relative_time"), hdr.index("edges_found")
        for line in f:
            row = [c.strip() for c in line.split(",")]
            if int(row[ti]) <= secs: best = int(row[ei])
    return best
def mw(a, b):
    u = lambda x, y: sum(1.0 if i > j else 0.5 if i == j else 0.0 for i in x for j in y)
    ua = u(a, b); umin = min(ua, len(a)*len(b) - ua); pool = a + b; c = n = 0
    for idx in itertools.combinations(range(len(pool)), len(a)):
        s = set(idx); x = [pool[i] for i in s]; y = [pool[i] for i in range(len(pool)) if i not in s]
        ux = u(x, y); n += 1
        if min(ux, len(x)*len(y) - ux) <= umin: c += 1
    return ua, c / n
os.makedirs(OUT, exist_ok=True)
for t in TRIALS:
    os.makedirs(os.path.join(OUT, t), exist_ok=True)
    for name in ("fuzzer_stats", "plot_data"):
        src = os.path.join(BASE, t, "default", name)
        if os.path.exists(src): shutil.copy2(src, os.path.join(OUT, t, name))
rows = ["trial,start_time_local,run_time_s,execs_done,avg_exec_per_s,final_edges,edges_6h,cmdline"]
print(f"{'trial':8} {'start':17} {'run_s':>7} {'avg ex/s':>9} {'edges':>6} {'@6h':>6}")
for t in TRIALS:
    s = stats(t)
    st = datetime.datetime.fromtimestamp(int(s["start_time"])).strftime("%Y-%m-%d %H:%M")
    rt, ex = int(s["run_time"]), int(s["execs_done"]); avg = ex / rt if rt else 0
    e6 = edges_at(t, 21600)
    print(f"{t:8} {st:17} {rt:7} {avg:9.1f} {s.get('edges_found','?'):>6} {str(e6):>6}")
    rows.append(f"{t},{st},{rt},{ex},{avg:.1f},{s.get('edges_found','')},{e6},\"{s.get('command_line','')}\"")
open(os.path.join(OUT, "run_conditions.csv"), "w").write("\n".join(rows) + "\n")
nd = [t for t in TRIALS if t.startswith("nd_")]; dd = [t for t in TRIALS if t.startswith("dd_")]
for label, ds in (("as submitted (no dd_3)", [t for t in dd if t != "dd_3"]), ("all dd trials", dd)):
    a = [edges_at(t, 21600) for t in nd]; b = [edges_at(t, 21600) for t in ds]
    U, p = mw(a, b)
    print(f"\n{label}: nd median={statistics.median(a)}  dd n={len(b)} median={statistics.median(b)}")
    print(f"  Mann-Whitney U={U:.1f}  exact two-sided p={p:.4f}")
