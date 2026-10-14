#!/usr/bin/env python3
"""
Visualize the cardiac wave written by `cardiacsim -w <freq>`.

    mpirun -n 4 ./cardiacsim -n 256 -t 300 -x 2 -y 2 -w 10
    python3 viz_wave.py            # reads ./frames, writes ./viz/*

Outputs (in ./viz):
  wave_strip.png   six snapshots in time order, one shared colour scale
  wave.gif         animation of every snapshot
  decomposition.png  the last snapshot with each MPI process' block outlined
Requires: numpy, matplotlib, pillow.
"""
import glob
import os
import re
import sys
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.patches import Rectangle
from PIL import Image

FRAMES = sys.argv[1] if len(sys.argv) > 1 else "frames"
OUT = "viz"
os.makedirs(OUT, exist_ok=True)

# one-hue sequential ramp (blue, light -> dark): near-zero voltage recedes, high voltage is dark
RAMP = ["#cde2fb", "#9ec5f4", "#6da7ec", "#3987e5", "#256abf", "#184f95", "#0d366b"]
CMAP = LinearSegmentedColormap.from_list("excitation", RAMP)
SURFACE, INK, INK2, MUTED = "#fcfcfb", "#0b0b0b", "#52514e", "#898781"
plt.rcParams.update({
    "font.family": "sans-serif",
    "font.sans-serif": ["DejaVu Sans"],
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE,
    "text.color": INK, "axes.labelcolor": INK2,
    "xtick.color": MUTED, "ytick.color": MUTED,
})

# ---- read the per-rank block files ----
pat = re.compile(r"f(\d+)_r(\d+)\.bin$")
by_frame = defaultdict(list)
for path in glob.glob(os.path.join(FRAMES, "f*_r*.bin")):
    mt = pat.search(path)
    if mt:
        by_frame[int(mt.group(1))].append((int(mt.group(2)), path))
if not by_frame:
    sys.exit(f"No frame files found in '{FRAMES}/'. Run cardiacsim with -w first.")


def load(frame):
    """Assemble the full N x N grid for one snapshot; also return each rank's block."""
    grid, blocks = None, {}
    for rank, path in sorted(by_frame[frame]):
        with open(path, "rb") as f:
            N, rbeg, cbeg, m, nn = np.frombuffer(f.read(20), dtype=np.int32)
            data = np.frombuffer(f.read(), dtype=np.float64).reshape(m, nn)
        if grid is None:
            grid = np.zeros((N, N))
        grid[rbeg - 1:rbeg - 1 + m, cbeg - 1:cbeg - 1 + nn] = data
        blocks[rank] = (rbeg - 1, cbeg - 1, m, nn)
    return grid, blocks


frames = sorted(by_frame)
grids = []
blocks = None
for fr in frames:
    g, blocks = load(fr)
    grids.append(g)
N = grids[0].shape[0]
nranks = len(blocks)
print(f"{len(frames)} snapshots, {N}x{N} grid, {nranks} MPI process(es)")

# time label: snapshot k was taken at k * snap_freq (the freq itself is not stored; show frame number)
def style(ax, title):
    ax.set_title(title, fontsize=10, color=INK2, loc="left")
    ax.set_xticks([]); ax.set_yticks([])
    for s in ax.spines.values():
        s.set_visible(False)


# ---- 1. filmstrip ----
pick = np.unique(np.linspace(0, len(frames) - 1, min(6, len(frames))).round().astype(int))
fig, axes = plt.subplots(1, len(pick), figsize=(2.2 * len(pick) + 1.0, 3.0), squeeze=False,
                         constrained_layout=True)
im = None
for ax, k in zip(axes[0], pick):
    im = ax.imshow(grids[k], cmap=CMAP, vmin=0, vmax=1, interpolation="nearest")
    style(ax, f"snapshot {frames[k]}")
cbar = fig.colorbar(im, ax=axes.ravel().tolist(), fraction=0.025, pad=0.02)
cbar.set_label("excitation E (voltage)", color=INK2)
cbar.outline.set_visible(False)
fig.suptitle("Excitation wave spreading across the heart tissue grid", x=0.01, ha="left",
             fontsize=12, color=INK)
fig.savefig(os.path.join(OUT, "wave_strip.png"), dpi=160, bbox_inches="tight")
plt.close(fig)

# ---- 2. animated GIF ----
pil = []
for k, g in enumerate(grids):
    fig, ax = plt.subplots(figsize=(4.4, 4.6))
    ax.imshow(g, cmap=CMAP, vmin=0, vmax=1, interpolation="nearest")
    style(ax, f"snapshot {frames[k]} of {frames[-1]}")
    fig.canvas.draw()
    pil.append(Image.fromarray(np.asarray(fig.canvas.buffer_rgba())[..., :3]).convert("P", palette=Image.ADAPTIVE))
    plt.close(fig)
pil[0].save(os.path.join(OUT, "wave.gif"), save_all=True, append_images=pil[1:], duration=150, loop=0)

# ---- 3. decomposition: who owns which part of the grid ----
fig, ax = plt.subplots(figsize=(5.2, 5.2))
ax.imshow(grids[-1], cmap=CMAP, vmin=0, vmax=1, interpolation="nearest")
for rank, (r0, c0, m, nn) in sorted(blocks.items()):
    ax.add_patch(Rectangle((c0 - 0.5, r0 - 0.5), nn, m, fill=False, ec=INK, lw=1.4))
    ax.text(c0 + nn / 2, r0 + m / 2, f"rank {rank}\n{m}x{nn}", ha="center", va="center",
            fontsize=9, color=INK,
            bbox=dict(boxstyle="round,pad=0.25", fc=SURFACE, ec="none", alpha=0.85))
style(ax, f"MPI domain decomposition ({nranks} processes)")
fig.savefig(os.path.join(OUT, "decomposition.png"), dpi=160, bbox_inches="tight")
plt.close(fig)

print("wrote", ", ".join(os.path.join(OUT, f) for f in ("wave_strip.png", "wave.gif", "decomposition.png")))
