#!/usr/bin/env python3
"""
Plot the three performance studies from results.csv (written by run_studies.sh).

    python3 plot_results.py [results.csv]      # writes ./viz/strong_scaling.png,
                                               #        comm_overhead.png, hybrid.png
CSV columns: study,N,P,px,py,threads,no_comm,gflops
"""
import csv
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

SRC = sys.argv[1] if len(sys.argv) > 1 else "results.csv"
OUT = "viz"
os.makedirs(OUT, exist_ok=True)

# categorical slots 1 and 2 (validated adjacent pair), neutral ink for reference lines
BLUE, ORANGE = "#2a78d6", "#eb6834"
SURFACE, INK, INK2, MUTED, GRID = "#fcfcfb", "#0b0b0b", "#52514e", "#898781", "#e1e0d9"
plt.rcParams.update({
    "font.family": "sans-serif", "font.sans-serif": ["DejaVu Sans"],
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE,
    "text.color": INK, "axes.labelcolor": INK2,
    "xtick.color": MUTED, "ytick.color": MUTED,
    "axes.edgecolor": "#c3c2b7", "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.8,
    "axes.axisbelow": True,
})


def clean(ax):
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    ax.tick_params(length=0)


rows = []
with open(SRC) as f:
    for r in csv.DictReader(f):
        try:
            rows.append(dict(study=r["study"], N=int(r["N"]), P=int(r["P"]), px=int(r["px"]),
                             py=int(r["py"]), th=int(r["threads"]), nc=int(r["no_comm"]),
                             g=float(r["gflops"])))
        except (ValueError, KeyError):
            pass  # skip empty/failed runs


def sel(study, **kw):
    return [r for r in rows if r["study"] == study and all(r[k] == v for k, v in kw.items())]


# ---------------- (a) strong scaling ----------------
strong = sel("strong")
serial = sel("serial")
if strong:
    Ps = sorted({r["P"] for r in strong})
    one_d = {r["P"]: r["g"] for r in strong if r["px"] == 1}                       # strips
    two_d = {}
    for r in strong:
        if r["px"] > 1 and r["py"] > 1 and r["g"] > two_d.get(r["P"], (0, ""))[0]:
            two_d[r["P"]] = (r["g"], f'{r["px"]}x{r["py"]}')                        # best block layout
    fig, ax = plt.subplots(figsize=(6.6, 4.2))
    base = one_d.get(1) or strong[0]["g"]
    ax.plot(Ps, [base * p for p in Ps], ls="--", lw=1.4, color=MUTED)
    ax.text(Ps[-1], base * Ps[-1], " ideal", color=INK2, va="center", fontsize=9)
    if serial:
        ax.axhline(serial[0]["g"], color=MUTED, lw=1, ls=":")
        ax.text(Ps[-1], serial[0]["g"], "original serial code ", color=INK2, ha="right",
                va="bottom", fontsize=9)
    pts1 = sorted(one_d.items())
    ax.plot([p for p, _ in pts1], [g for _, g in pts1], "-o", color=BLUE, lw=2, ms=7,
            mec=SURFACE, mew=2, label="1D strips (1 x P)")
    if two_d:
        pts2 = sorted((p, v[0]) for p, v in two_d.items())
        ax.plot([p for p, _ in pts2], [g for _, g in pts2], "-o", color=ORANGE, lw=2, ms=7,
                mec=SURFACE, mew=2, label="best 2D blocks")
        for p, (g, lab) in sorted(two_d.items()):
            ax.annotate(lab, (p, g), textcoords="offset points", xytext=(0, 10), ha="center",
                        fontsize=8, color=INK2)
    ax.set_xscale("log", base=2)
    ax.set_xticks(Ps)
    ax.get_xaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())
    ax.set_xlabel("MPI processes (P)")
    ax.set_ylabel("Sustained Gflops")
    ax.set_title("Strong scaling, N = 1024 (no OpenMP)", loc="left", fontsize=11, color=INK)
    ax.legend(frameon=False, loc="upper left")
    clean(ax)
    fig.savefig(os.path.join(OUT, "strong_scaling.png"), dpi=160, bbox_inches="tight")
    plt.close(fig)
    print("wrote viz/strong_scaling.png")

# ---------------- (b) communication overhead ----------------
comm = sel("comm")
if comm:
    Ns = sorted({r["N"] for r in comm}, reverse=True)
    labels, over = [], []
    for n in Ns:
        w = [r["g"] for r in comm if r["N"] == n and r["nc"] == 0]
        wo = [r["g"] for r in comm if r["N"] == n and r["nc"] == 1]
        if w and wo and wo[0] > 0:
            labels.append(str(n))
            over.append(max(0.0, 100.0 * (1.0 - w[0] / wo[0])))   # share of run time spent communicating
    fig, ax = plt.subplots(figsize=(6.6, 4.2))
    bars = ax.bar(labels, over, color=BLUE, width=0.62)
    ax.axhline(25, color=MUTED, lw=1.2, ls="--")
    ax.text(-0.45, 25, " 25% threshold", ha="left", va="bottom", fontsize=9, color=INK2)
    for b, v in zip(bars, over):
        ax.text(b.get_x() + b.get_width() / 2, v, f"{v:.0f}%", ha="center", va="bottom",
                fontsize=9, color=INK2)
    ax.set_xlabel("Mesh size N (work halves at each step to the right)")
    ax.set_ylabel("Communication overhead (% of run time)")
    ax.set_title("Communication overhead vs problem size", loc="left", fontsize=11, color=INK)
    ax.set_ylim(0, max(max(over) * 1.2, 30))
    ax.grid(axis="x", visible=False)
    clean(ax)
    fig.savefig(os.path.join(OUT, "comm_overhead.png"), dpi=160, bbox_inches="tight")
    plt.close(fig)
    print("wrote viz/comm_overhead.png")

# ---------------- (c) MPI + OpenMP ----------------
hyb = sorted(sel("hybrid"), key=lambda r: -r["P"])
if hyb:
    labels = [f'{r["P"]} procs\nx {r["th"]} threads' for r in hyb]
    vals = [r["g"] for r in hyb]
    best = vals.index(max(vals))
    fig, ax = plt.subplots(figsize=(6.6, 4.2))
    bars = ax.bar(labels, vals, color=BLUE, width=0.62)
    for i, (b, v) in enumerate(zip(bars, vals)):
        ax.text(b.get_x() + b.get_width() / 2, v, f"{v:.1f}" + ("  fastest" if i == best else ""),
                ha="center", va="bottom", fontsize=9, color=INK2)
    ax.set_ylabel("Sustained Gflops")
    ax.set_title("MPI + OpenMP combinations on 16 cores (1D decomposition, N = 1024)",
                 loc="left", fontsize=11, color=INK)
    ax.set_ylim(0, max(vals) * 1.18)
    ax.grid(axis="x", visible=False)
    clean(ax)
    fig.savefig(os.path.join(OUT, "hybrid.png"), dpi=160, bbox_inches="tight")
    plt.close(fig)
    print("wrote viz/hybrid.png")

if not (strong or comm or hyb):
    sys.exit("results.csv had no usable rows")
