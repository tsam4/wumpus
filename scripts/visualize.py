#!/usr/bin/env python3
"""
visualizer for wumpus tracker output.

cell appearance:
  light orange fill  -- sensor detected something in this cell
  green rectangle    -- true wumpus location (dataset 1 only)
  blue filled circle -- marginal (BP) prediction

usage: python scripts/visualize.py <dataset_id>   (default: 1)
saves animated GIF to results/out_d<N>_anim.gif and opens interactive window.
"""

import sys
import os
import glob
from pathlib import Path

try:
    import numpy as np
    NP_AVAILABLE = True
except ImportError:
    NP_AVAILABLE = False

try:
    import matplotlib
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    import matplotlib.lines as mlines
    from matplotlib.animation import FuncAnimation, PillowWriter
    MATPLOTLIB_AVAILABLE = NP_AVAILABLE
except ImportError:
    MATPLOTLIB_AVAILABLE = False

try:
    from PIL import Image
    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False


# ---------------------------------------------------------------------------
# visual constants
# ---------------------------------------------------------------------------
_C_EMPTY        = "#f5f5f5"   # plain empty cell
_C_DETECTION    = "#fde8c8"   # light orange -- sensor triggered
_C_GRID         = "#c0c0c0"   # grid lines
_C_PREDICTION   = "#2178c4"   # blue circle -- marginal prediction
_C_TRUE_OUTLINE = "#27ae60"   # green box -- true wumpus location

_CIRCLE_RADIUS  = 0.33        # fraction of cell width
_OUTLINE_LW     = 3.5         # line width for green outline

# project root is two levels above this script (scripts/visualize.py)
_PROJECT_ROOT = Path(__file__).parent.parent
_RESULTS_DIR  = _PROJECT_ROOT / "results"


# ---------------------------------------------------------------------------
# file I/O
# ---------------------------------------------------------------------------

def load_grid_file(filepath):
    grid = []
    with open(filepath, 'r') as f:
        for line in f:
            grid.append([int(v) for v in line.strip().split()])
    return np.array(grid, dtype=int) if NP_AVAILABLE else grid


def grid_value(grid, y, x):
    return int(grid[y, x]) if NP_AVAILABLE else grid[y][x]


def load_trajectory_file(filepath):
    traj = []
    with open(filepath) as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 2:
                traj.append((int(parts[0]), int(parts[1])))
    return traj


def find_dataset_dir(dataset_id):
    p = _PROJECT_ROOT / f"Datasets-20260506/dataset{dataset_id}"
    if p.exists():
        return str(p)
    print(f"Error: Dataset {dataset_id} not found at {p}")
    return None


def find_ground_truth_file(dataset_id):
    # ground truth only available for dataset 1
    p = (_PROJECT_ROOT
         / "Ground truth for dataset1-20260506"
         / "wumpus_trajectory.txt")
    return str(p) if p.exists() and dataset_id == 1 else None


def load_all_grids(dataset_dir):
    files = sorted(glob.glob(os.path.join(dataset_dir, "data_file*.txt")))
    if not files:
        print(f"Error: no data files in {dataset_dir}")
        return None
    grids = []
    for f in files:
        try:
            grids.append(load_grid_file(f))
        except Exception as e:
            print(f"Error loading {f}: {e}")
            return None
    return grids


# ---------------------------------------------------------------------------
# figure factory
# ---------------------------------------------------------------------------

def _make_figure(rows, cols):
    cell_px = 60
    dpi     = 100
    fig_w = max(6.5, cols * cell_px / dpi + 3.4)  # extra width for legend
    fig_h = max(4.5, rows * cell_px / dpi + 1.6)
    fig, ax = plt.subplots(figsize=(fig_w, fig_h))
    fig.patch.set_facecolor("white")
    return fig, ax


# ---------------------------------------------------------------------------
# drawing primitives
# ---------------------------------------------------------------------------

def _draw_cells(ax, obs_grid, rows, cols):
    for y in range(rows):
        for x in range(cols):
            colour = _C_DETECTION if grid_value(obs_grid, y, x) else _C_EMPTY
            ax.add_patch(mpatches.FancyBboxPatch(
                (x - 0.5, y - 0.5), 1.0, 1.0,
                boxstyle="square,pad=0",
                linewidth=0, facecolor=colour, zorder=1,
            ))


def _draw_grid_lines(ax, rows, cols):
    for xi in range(cols + 1):
        ax.axvline(xi - 0.5, color=_C_GRID, linewidth=0.7, zorder=2)
    for yi in range(rows + 1):
        ax.axhline(yi - 0.5, color=_C_GRID, linewidth=0.7, zorder=2)


def _draw_true_outline(ax, tx, ty):
    if tx < 0:
        return
    ax.add_patch(mpatches.FancyBboxPatch(
        (tx - 0.46, ty - 0.46), 0.92, 0.92,
        boxstyle="square,pad=0",
        linewidth=_OUTLINE_LW,
        edgecolor=_C_TRUE_OUTLINE,
        facecolor="none",
        zorder=4,
    ))


def _draw_prediction_circle(ax, mx, my):
    if mx < 0:
        return
    ax.add_patch(mpatches.Circle(
        (mx, my), _CIRCLE_RADIUS,
        facecolor=_C_PREDICTION,
        edgecolor="white",
        linewidth=1.5,
        zorder=3,
    ))


def _style_ax(ax, rows, cols, timestep):
    ax.set_facecolor("white")
    ax.set_xlim(-0.5, cols - 0.5)
    ax.set_ylim(rows - 0.5, -0.5)
    ax.set_xticks(range(cols))
    ax.set_yticks(range(rows))
    ax.tick_params(labelsize=8, length=0)
    ax.set_xlabel("Column (x)", fontsize=9, labelpad=6)
    ax.set_ylabel("Row (y)",    fontsize=9, labelpad=6)
    ax.set_title(f"Wumpus Tracker  \u2014  t = {timestep}",
                 fontsize=11, fontweight="bold", pad=10)
    for spine in ax.spines.values():
        spine.set_edgecolor(_C_GRID)


def _build_legend():
    return [
        mpatches.Patch(facecolor=_C_EMPTY,     edgecolor=_C_GRID,
                       label="Empty cell"),
        mpatches.Patch(facecolor=_C_DETECTION, edgecolor=_C_GRID,
                       label="Sensor detection"),
        mlines.Line2D([], [], marker='o', color='w',
                      markerfacecolor=_C_PREDICTION, markeredgecolor='white',
                      markersize=10, label="Marginal prediction (BP)"),
        mpatches.Patch(facecolor="none",
                       edgecolor=_C_TRUE_OUTLINE, linewidth=_OUTLINE_LW,
                       label="True wumpus location"),
    ]


# ---------------------------------------------------------------------------
# render a complete frame onto an existing axes
# ---------------------------------------------------------------------------

def _render_into_ax(ax, obs_grid, rows, cols, mx, my, tx, ty, timestep):
    """Clear ax and draw one complete timestep."""
    ax.cla()
    _draw_cells(ax, obs_grid, rows, cols)
    _draw_grid_lines(ax, rows, cols)
    _draw_prediction_circle(ax, mx, my)
    _draw_true_outline(ax, tx, ty)
    _style_ax(ax, rows, cols, timestep)
    ax.legend(handles=_build_legend(),
              loc="upper left", bbox_to_anchor=(1.01, 1),
              fontsize=8, framealpha=0.95,
              edgecolor=_C_GRID, borderaxespad=0)


# ---------------------------------------------------------------------------
# single-frame save / show
# ---------------------------------------------------------------------------

def visualize_step(grids, marginal_trajectory, timestep,
                   output_path=None, ground_truth=None):
    """Render one timestep to a file or interactive window."""
    if timestep >= len(grids):
        print(f"Error: timestep {timestep} out of range")
        return

    obs_grid = grids[timestep]
    rows = len(obs_grid)
    cols = len(obs_grid[0]) if rows else 0
    mx, my = marginal_trajectory[timestep] if timestep < len(marginal_trajectory) else (-1, -1)
    tx, ty = (ground_truth[timestep] if ground_truth and timestep < len(ground_truth)
               else (-1, -1))

    if not MATPLOTLIB_AVAILABLE:
        _ascii_frame(obs_grid, rows, cols, mx, my, tx, ty, timestep)
        return

    fig, ax = _make_figure(rows, cols)
    _render_into_ax(ax, obs_grid, rows, cols, mx, my, tx, ty, timestep)
    fig.tight_layout(pad=1.2)

    if output_path:
        fig.savefig(output_path, dpi=100, bbox_inches="tight", facecolor="white")
        print(f"Saved frame -> {output_path}")
    else:
        plt.show()
    plt.close(fig)


def _ascii_frame(obs_grid, rows, cols, mx, my, tx, ty, timestep):
    print(f"\n{'='*60}\nTimestep {timestep}\n{'='*60}")
    for y in range(rows):
        row = ""
        for x in range(cols):
            det  = grid_value(obs_grid, y, x)
            pred = (x == mx and y == my)
            true = (x == tx and y == ty)
            if true and pred and det: row += "!"
            elif true and pred:       row += "*"
            elif true and det:        row += "T"
            elif pred and det:        row += "P"
            elif true:                row += "t"
            elif pred:                row += "o"
            elif det:                 row += "D"
            else:                     row += "."
        print(row)
    print(f"Marginal: ({mx},{my})" + (f"  True: ({tx},{ty})" if tx >= 0 else ""))


# ---------------------------------------------------------------------------
# GIF export
# ---------------------------------------------------------------------------

def save_gif(grids, marginal_trajectory, gif_path, ground_truth=None, fps=1.5):
    """Save all timesteps as an animated GIF."""
    if not MATPLOTLIB_AVAILABLE:
        print("Cannot save GIF: matplotlib/numpy not available.")
        return

    rows, cols = grids[0].shape
    fig, ax    = _make_figure(rows, cols)

    def _update(frame):
        obs_grid = grids[frame]
        r, c     = obs_grid.shape
        mx, my   = (marginal_trajectory[frame]
                    if frame < len(marginal_trajectory) else (-1, -1))
        tx, ty   = (ground_truth[frame]
                    if ground_truth and frame < len(ground_truth) else (-1, -1))
        _render_into_ax(ax, obs_grid, r, c, mx, my, tx, ty, frame)
        fig.tight_layout(pad=1.2)
        return ax.get_children()

    anim = FuncAnimation(
        fig, _update,
        frames=len(grids),
        interval=int(1000 / fps),
        blit=False,
        repeat=True,
    )
    anim.save(gif_path, writer=PillowWriter(fps=fps), dpi=100)
    plt.close(fig)
    print(f"Saved GIF  -> {gif_path}")


# ---------------------------------------------------------------------------
# interactive window
# ---------------------------------------------------------------------------

def play_visualization(grids, marginal_trajectory, ground_truth=None):
    rows, cols = grids[0].shape
    fig, ax    = _make_figure(rows, cols)

    def _update(frame):
        obs_grid = grids[frame]
        r, c     = obs_grid.shape
        mx, my   = (marginal_trajectory[frame]
                    if frame < len(marginal_trajectory) else (-1, -1))
        tx, ty   = (ground_truth[frame]
                    if ground_truth and frame < len(ground_truth) else (-1, -1))
        _render_into_ax(ax, obs_grid, r, c, mx, my, tx, ty, frame)
        fig.tight_layout(pad=1.2)
        return ax.get_children()

    anim = FuncAnimation(  # noqa: F841 -- keep ref to prevent GC
        fig, _update,
        frames=len(grids),
        interval=800,
        blit=False,
        repeat=False,
    )
    plt.show()


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    dataset_id = 1
    if len(sys.argv) > 1:
        try:
            dataset_id = int(sys.argv[1])
        except ValueError:
            print(f"Invalid dataset ID: {sys.argv[1]}")
            sys.exit(1)

    if not 1 <= dataset_id <= 3:
        print(f"Dataset ID must be 1-3, got {dataset_id}")
        sys.exit(1)

    dataset_dir = find_dataset_dir(dataset_id)
    if not dataset_dir:
        sys.exit(1)

    # ensure results/ exists
    _RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    out_prefix    = _RESULTS_DIR / f"out_d{dataset_id}"
    marginal_file = str(out_prefix) + "_marginal.txt"
    gif_path      = str(out_prefix) + "_anim.gif"

    print(f"Loading dataset {dataset_id}...")
    grids = load_all_grids(dataset_dir)
    if not grids:
        sys.exit(1)
    print(f"Loaded {len(grids)} observation grids")

    if not os.path.exists(marginal_file):
        print("Error: output file not found")
        print(f"  {os.path.basename(marginal_file)} (exists: False)")
        print("Run 'make run' first.")
        sys.exit(1)

    marginal_traj = load_trajectory_file(marginal_file)
    print(f"Loaded marginal trajectory: {len(marginal_traj)} points")

    ground_truth = None
    gt_file = find_ground_truth_file(dataset_id)
    if gt_file:
        ground_truth = load_trajectory_file(gt_file)
        if ground_truth:
            print(f"Loaded ground truth: {len(ground_truth)} points")

    if MATPLOTLIB_AVAILABLE:
        save_gif(grids, marginal_traj, gif_path, ground_truth, fps=1.5)
    else:
        print("Cannot save GIF: matplotlib/numpy not available.")

    if MATPLOTLIB_AVAILABLE:
        print("\nLaunching interactive visualization...")
        play_visualization(grids, marginal_traj, ground_truth)
    else:
        print("\nASCII fallback (. empty  D detect  o pred  t true  P pred+det  T true+det  * true+pred  ! all):")
        for t in range(len(grids)):
            _ascii_frame(grids[t], *grids[t].shape,
                         *(marginal_traj[t] if t < len(marginal_traj) else (-1, -1)),
                         *(ground_truth[t]  if ground_truth and t < len(ground_truth) else (-1, -1)),
                         t)


if __name__ == "__main__":
    main()
