#!/usr/bin/env python3
import os, statistics
from functools import lru_cache

BASE = "campaign"
NODICT = ["nd_1", "nd_2", "nd_3", "nd_4", "nd_5"]
DICT   = ["dd_1", "dd_2", "dd_4", "dd_5", "dd_6"]

def stat(trial, key):
    p = os.path.join(BASE, trial, "default", "fuzzer_stats")
    with open(p) as f:
        for line in f:
            if line.startswith(key):
                return line.split(":", 1)[1].strip()
    return None

def edges(trial):
    for k in ("edges_found", "map_size"):
        v = stat(trial, k)
        if v is not None:
            return float(v.rstrip("%"))
    raise SystemExit(f"no coverage field in {trial}/fuzzer_stats")

def summarize(name, xs):
    xs = sorted(xs)
    med = statistics.median(xs)
    q1  = statistics.median(xs[:len(xs)//2])
    q3  = statistics.median(xs[(len(xs)+1)//2:])
    print(f"  {name}: n={len(xs)}  values={[int(x) for x in xs]}")
    print(f"        median={med:.0f}  IQR=[{q1:.0f}, {q3:.0f}]  range={xs[0]:.0f}-{xs[-1]:.0f}")
    return med

def mann_whitney_exact(a, b):
    n1, n2 = len(a), len(b)
    comb = sorted([(v, 0) for v in a] + [(v, 1) for v in b])
    ranks, i = [0.0]*len(comb), 0
    while i < len(comb):
        j = i
        while j+1 < len(comb) and comb[j+1][0] == comb[i][0]:
            j += 1
        avg = (i + j)/2.0 + 1
        for k in range(i, j+1):
            ranks[k] = avg
        i = j + 1
    R1 = sum(r for r, (v, g) in zip(ranks, comb) if g == 0)
    U1 = R1 - n1*(n1+1)/2.0
    U2 = n1*n2 - U1
    U  = min(U1, U2)

    @lru_cache(maxsize=None)
    def C(m, n, u):
        if u < 0: return 0
        if m == 0 or n == 0: return 1 if u == 0 else 0
        return C(m-1, n, u-n) + C(m, n-1, u)
    total = sum(C(n1, n2, u) for u in range(n1*n2 + 1))
    tail  = sum(C(n1, n2, u) for u in range(int(round(U)) + 1))
    p = min(1.0, 2*tail/total)
    return U1, U2, U, p

nd = [edges(t) for t in NODICT]
dd = [edges(t) for t in DICT]
print("=== Dictionary A/B — edge coverage at 6h (edges_found) ===")
m_nd = summarize("no-dict", nd)
m_dd = summarize("+dict  ", dd)
U1, U2, U, p = mann_whitney_exact(nd, dd)
print(f"\n  Mann-Whitney U = {U:.1f}  (U_nodict={U1:.1f}, U_dict={U2:.1f})")
print(f"  exact two-sided p = {p:.4f}")
print(f"  median difference (+dict - no-dict) = {m_dd - m_nd:+.0f} edges "
      f"({100*(m_dd-m_nd)/m_nd:+.1f}%)")
print("  => " + ("dictionary significantly changes coverage (p<0.05)" if p < 0.05
                  else "no significant difference at alpha=0.05 (n=5 is low-powered)"))

def curve(trial):
    p = os.path.join(BASE, trial, "default", "plot_data")
    pts = []
    try:
        with open(p) as f:
            header = [h.strip() for h in f.readline().lstrip("#").strip().split(",")]
            ti = header.index("relative_time")
            ei = header.index("edges_found") if "edges_found" in header else header.index("map_size")
            for line in f:
                c = line.strip().split(",")
                if len(c) > max(ti, ei):
                    pts.append((float(c[ti]), float(c[ei].rstrip("%"))))
    except (FileNotFoundError, ValueError):
        return []
    return pts

def at(pts, t):
    v = None
    for (tt, ee) in pts:
        if tt <= t: v = ee
        else: break
    return v

try:
    nd_c = [curve(t) for t in NODICT]
    dd_c = [curve(t) for t in DICT]
    if all(nd_c) and all(dd_c):
        with open("curves.csv", "w") as f:
            f.write("time_s,nodict_mean_edges,dict_mean_edges\n")
            for t in range(0, 21601, 600):
                nv = [x for x in (at(c, t) for c in nd_c) if x is not None]
                dv = [x for x in (at(c, t) for c in dd_c) if x is not None]
                if nv and dv:
                    f.write(f"{t},{sum(nv)/len(nv):.1f},{sum(dv)/len(dv):.1f}\n")
        print("\n  wrote curves.csv (mean coverage vs time per arm)")
except Exception as e:
    print(f"\n  (curve CSV skipped: {e})")
