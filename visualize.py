#!/usr/bin/env python3
"""
Wumpus Trajectory Visualizer

Displays measurements (detections) and predicted Wumpus location at each timestep.
Shows the marginal (belief-propagation argmax) trajectory.

Always saves an animated GIF to out_d<N>_anim.gif.
Interactive window is shown when matplotlib is available.

Usage:
    python visualize.py <dataset_id>

where dataset_id is 1-3 (default: 1)
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
    from matplotlib.animation import FuncAnimation, PillowWriter
    MATPLOTLIB_AVAILABLE = NP_AVAILABLE
except ImportError:
    MATPLOTLIB_AVAILABLE = False

try:
    from PIL import Image, ImageDraw, ImageFont
    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False


# ---------------------------------------------------------------------------
# Colour palette
# ---------------------------------------------------------------------------
_C_EMPTY           = "#f0f0f0"   # empty, undetected cell
_C_DETECTION       = "#4a90d9"   # sensor triggered here              (blue)
_C_TRUE            = "#e8a020"   # ground-truth wumpus location       (amber)
_C_MARGINAL        = "#2ca02c"   # marginal-argmax prediction         (green)
_C_OVERLAP         = "#9b59b6"   # marginal prediction ON a detection (purple)
_C_TRUE_DETECT     = "#e84040"   # ground-truth ON a detection        (red)
_C_TRUE_MARGINAL   = "#f5c842"   # ground-truth ON marginal (correct) (bright gold)
_C_TRUE_MAR_DET    = "#ff6f00"   # all three overlap                  (deep orange)
_C_GRID            = "#cccccc"   # grid line colour


# ---------------------------------------------------------------------------
# File I/O helpers
# ---------------------------------------------------------------------------

def load_grid_file(filepath):
    """Load a single observation grid from a data file."""
    grid = []
    with open(filepath, 'r') as f:
        for line in f:
            row = [int(x) for x in line.strip().split()]
            grid.append(row)
    if NP_AVAILABLE:
        return np.array(grid, dtype=int)
    return grid


def grid_value(grid, y, x):
    if NP_AVAILABLE:
        return int(grid[y, x])
    return grid[y][x]


def load_trajectory_file(filepath):
    """Load a trajectory file (marginal)."""
    trajectory = []
    with open(filepath, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 2:
                x, y = int(parts[0]), int(parts[1])
                trajectory.append((x, y))
    return trajectory


def find_dataset_dir(dataset_id):
    """Find the dataset directory for the given ID."""
    script_dir = Path(__file__).parent
    dataset_dir = script_dir / f"Datasets-20260506/dataset{dataset_id}"
    if dataset_dir.exists():
        return str(dataset_dir)
    print(f"Error: Dataset {dataset_id} not found at {dataset_dir.name}")
    return None


def find_ground_truth_file(dataset_id):
    script_dir = Path(__file__).parent
    ground_truth_path = (
        script_dir / "Ground truth for dataset1-20260506" / "wumpus_trajectory.txt"
    )
    if ground_truth_path.exists() and dataset_id == 1:
        return str(ground_truth_path)
    return None


def load_all_grids(dataset_dir):
    """Load all observation grids from a dataset directory."""
    data_files = sorted(glob.glob(os.path.join(dataset_dir, "data_file*.txt")))
    if not data_files:
        print(f"Error: No data files found in {dataset_dir}")
        return None
    grids = []
    for filepath in data_files:
        try:
            grids.append(load_grid_file(filepath))
        except Exception as e:
            print(f"Error loading {filepath}: {e}")
            return None
    return grids


# ---------------------------------------------------------------------------
# Core colour-block rendering
# ---------------------------------------------------------------------------

def _cell_colour(obs_val, is_true, is_marginal):
    """
    Return the fill colour for one grid cell.

    Overlap combinations handled explicitly so no information is lost:
      true + marginal + detection -> _C_TRUE_MAR_DET  (deep orange)
      true + marginal             -> _C_TRUE_MARGINAL  (bright gold  = correct hit)
      true + detection            -> _C_TRUE_DETECT    (red)
      marginal + detection        -> _C_OVERLAP        (purple)
      true only                  -> _C_TRUE            (amber)
      marginal only              -> _C_MARGINAL        (green)
      detection only             -> _C_DETECTION       (blue)
      empty                      -> _C_EMPTY
    """
    det = bool(obs_val)
    if is_true and is_marginal and det:
        return _C_TRUE_MAR_DET
    if is_true and is_marginal:
        return _C_TRUE_MARGINAL
    if is_true and det:
        return _C_TRUE_DETECT
    if is_marginal and det:
        return _C_OVERLAP
    if is_true:
        return _C_TRUE
    if is_marginal:
        return _C_MARGINAL
    if det:
        return _C_DETECTION
    return _C_EMPTY


def _build_colour_image(obs_grid, rows, cols, mx, my, tx, ty):
    """Build an (rows x cols x 3) float32 RGB array."""
    from matplotlib.colors import to_rgb
    img = np.zeros((rows, cols, 3), dtype=np.float32)
    for y in range(rows):
        for x in range(cols):
            obs_val  = grid_value(obs_grid, y, x)
            is_true  = (x == tx and y == ty)
            is_marg  = (x == mx and y == my)
            img[y, x] = to_rgb(_cell_colour(obs_val, is_true, is_marg))
    return img


def _draw_grid_lines(ax, rows, cols):
    """Thin grey grid lines between cells."""
    for xi in range(cols + 1):
        ax.axvline(xi - 0.5, color=_C_GRID, linewidth=0.7, zorder=2)
    for yi in range(rows + 1):
        ax.axhline(yi - 0.5, color=_C_GRID, linewidth=0.7, zorder=2)


def _style_ax(ax, rows, cols, timestep):
    ax.set_facecolor("white")
    ax.set_xlim(-0.5, cols - 0.5)
    ax.set_ylim(rows - 0.5, -0.5)
    ax.set_xticks(range(cols))
    ax.set_yticks(range(rows))
    ax.tick_params(labelsize=8, length=0)
    ax.set_xlabel("Column (x)", fontsize=9, labelpad=6)
    ax.set_ylabel("Row (y)", fontsize=9, labelpad=6)
    ax.set_title(f"Observations  -  t = {timestep}",
                 fontsize=11, fontweight="bold", pad=10)
    for spine in ax.spines.values():
        spine.set_edgecolor(_C_GRID)


def _legend_patches():
    """Legend covering all eight possible cell states."""
    return [
        mpatches.Patch(color=_C_EMPTY,         label="Empty"),
        mpatches.Patch(color=_C_DETECTION,      label="Detection"),
        mpatches.Patch(color=_C_MARGINAL,       label="Marginal (BP)"),
        mpatches.Patch(color=_C_OVERLAP,        label="Marginal + Detection"),
        mpatches.Patch(color=_C_TRUE,           label="True location"),
        mpatches.Patch(color=_C_TRUE_DETECT,    label="True + Detection"),
        mpatches.Patch(color=_C_TRUE_MARGINAL,  label="True + Marginal  ✓"),
        mpatches.Patch(color=_C_TRUE_MAR_DET,   label="True + Marginal + Detection"),
    ]


# ---------------------------------------------------------------------------
# Figure/axes factory  (shared by static render and animation)
# ---------------------------------------------------------------------------

def _make_figure(rows, cols):
    cell_px = 60
    dpi     = 100
    fig_w   = max(6.0, cols * cell_px / dpi + 3.2)   # extra room for legend
    fig_h   = max(4.5, rows * cell_px / dpi + 1.6)

    fig, ax = plt.subplots(figsize=(fig_w, fig_h))
    fig.patch.set_facecolor("white")
    return fig, ax


# ---------------------------------------------------------------------------
# Static single-frame render
# ---------------------------------------------------------------------------

def _render_frame(obs_grid, rows, cols, mx, my, tx, ty, timestep,
                  output_path=None):
    fig, ax = _make_figure(rows, cols)

    img = _build_colour_image(obs_grid, rows, cols, mx, my, tx, ty)
    ax.imshow(img, origin="upper", interpolation="nearest",
              extent=(-0.5, cols - 0.5, rows - 0.5, -0.5), zorder=1)

    _draw_grid_lines(ax, rows, cols)
    _style_ax(ax, rows, cols, timestep)
    ax.legend(handles=_legend_patches(), loc="upper left",
              bbox_to_anchor=(1.01, 1), fontsize=7.5,
              framealpha=0.95, edgecolor=_C_GRID, borderaxespad=0)
    fig.tight_layout(pad=1.2)

    if output_path:
        fig.savefig(output_path, dpi=100, bbox_inches="tight", facecolor="white")
        print(f"Saved frame to {output_path}")
    else:
        plt.show()
    plt.close(fig)


# ---------------------------------------------------------------------------
# Public API  (visualize_step)
# ---------------------------------------------------------------------------

def visualize_step(grids, marginal_trajectory, timestep,
                   output_path=None, ground_truth=None):
    """Visualize a single timestep with observations and marginal prediction."""
    if timestep >= len(grids):
        print(f"Error: Timestep {timestep} out of range (max {len(grids)-1})")
        return

    obs_grid = grids[timestep]
    rows     = len(obs_grid)
    cols     = len(obs_grid[0]) if rows else 0

    mx, my = marginal_trajectory[timestep] if timestep < len(marginal_trajectory) else (-1, -1)
    tx, ty = (ground_truth[timestep]       if ground_truth and timestep < len(ground_truth)
               else (-1, -1))

    if MATPLOTLIB_AVAILABLE:
        _render_frame(obs_grid, rows, cols, mx, my, tx, ty, timestep, output_path)
    else:
        # ASCII fallback
        _ascii_frame(grids, marginal_trajectory, timestep, ground_truth)


def _ascii_frame(grids, marginal_trajectory, timestep, ground_truth):
    obs_grid = grids[timestep]
    rows     = len(obs_grid)
    cols     = len(obs_grid[0]) if rows else 0
    mx, my   = marginal_trajectory[timestep] if timestep < len(marginal_trajectory) else (-1, -1)
    tx, ty   = (ground_truth[timestep] if ground_truth and timestep < len(ground_truth)
                else (-1, -1))
    symbol_map = {
        _C_EMPTY: ".", _C_DETECTION: "D", _C_MARGINAL: "G",
        _C_OVERLAP: "P", _C_TRUE: "T", _C_TRUE_DETECT: "R",
        _C_TRUE_MARGINAL: "*", _C_TRUE_MAR_DET: "!",
    }
    print(f"\n{'='*60}\nTimestep {timestep}\n{'='*60}")
    for y in range(rows):
        row_str = ""
        for x in range(cols):
            c = _cell_colour(grid_value(obs_grid, y, x),
                             x == tx and y == ty,
                             x == mx and y == my)
            row_str += symbol_map.get(c, "?")
        print(row_str)
    print(f"Marginal: ({mx},{my})" + (f"  True: ({tx},{ty})" if tx >= 0 else ""))


# ---------------------------------------------------------------------------
# GIF export
# ---------------------------------------------------------------------------

def save_gif(grids, marginal_trajectory, gif_path, ground_truth=None,
             fps=1.5):
    """
    Render every timestep into an animated GIF saved at gif_path.
    fps controls playback speed (default 1.5 frames/sec = 667 ms/frame).
    """
    if not MATPLOTLIB_AVAILABLE:
        print("Cannot save GIF: matplotlib/numpy not available.")
        return

    rows, cols = grids[0].shape
    fig, ax    = _make_figure(rows, cols)

    blank    = np.ones((rows, cols, 3), dtype=np.float32)
    obs_img  = ax.imshow(blank, origin="upper", interpolation="nearest",
                         extent=(-0.5, cols - 0.5, rows - 0.5, -0.5), zorder=1)
    _draw_grid_lines(ax, rows, cols)
    _style_ax(ax, rows, cols, 0)
    ax.legend(handles=_legend_patches(), loc="upper left",
              bbox_to_anchor=(1.01, 1), fontsize=7.5,
              framealpha=0.95, edgecolor=_C_GRID, borderaxespad=0)
    fig.tight_layout(pad=1.2)

    def _update(frame):
        obs_grid = grids[frame]
        r, c     = obs_grid.shape
        mx, my   = (marginal_trajectory[frame]
                    if frame < len(marginal_trajectory) else (-1, -1))
        tx, ty   = (ground_truth[frame]
                    if ground_truth and frame < len(ground_truth) else (-1, -1))
        obs_img.set_data(_build_colour_image(obs_grid, r, c, mx, my, tx, ty))
        ax.set_title(f"Observations  -  t = {frame}",
                     fontsize=11, fontweight="bold", pad=10)
        return [obs_img]

    interval_ms = int(1000 / fps)
    anim = FuncAnimation(
        fig, _update,
        frames=len(grids),
        interval=interval_ms,
        blit=False,
        repeat=True,
    )

    writer = PillowWriter(fps=fps)
    anim.save(gif_path, writer=writer, dpi=100)
    plt.close(fig)
    print(f"Saved GIF  -> {gif_path}")


# ---------------------------------------------------------------------------
# Interactive animation window
# ---------------------------------------------------------------------------

def play_visualization(grids, marginal_trajectory, ground_truth=None):
    rows, cols = grids[0].shape
    fig, ax    = _make_figure(rows, cols)

    blank   = np.ones((rows, cols, 3), dtype=np.float32)
    obs_img = ax.imshow(blank, origin="upper", interpolation="nearest",
                        extent=(-0.5, cols - 0.5, rows - 0.5, -0.5), zorder=1)
    _draw_grid_lines(ax, rows, cols)
    _style_ax(ax, rows, cols, 0)
    ax.legend(handles=_legend_patches(), loc="upper left",
              bbox_to_anchor=(1.01, 1), fontsize=7.5,
              framealpha=0.95, edgecolor=_C_GRID, borderaxespad=0)
    fig.tight_layout(pad=1.2)

    def _update(frame):
        obs_grid = grids[frame]
        r, c     = obs_grid.shape
        mx, my   = (marginal_trajectory[frame]
                    if frame < len(marginal_trajectory) else (-1, -1))
        tx, ty   = (ground_truth[frame]
                    if ground_truth and frame < len(ground_truth) else (-1, -1))
        obs_img.set_data(_build_colour_image(obs_grid, r, c, mx, my, tx, ty))
        ax.set_title(f"Observations  -  t = {frame}",
                     fontsize=11, fontweight="bold", pad=10)
        return [obs_img]

    # Must keep reference to prevent GC destroying the animation.
    anim = FuncAnimation(  # noqa: F841
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

    if dataset_id < 1 or dataset_id > 3:
        print(f"Dataset ID must be 1-3, got {dataset_id}")
        sys.exit(1)

    dataset_dir = find_dataset_dir(dataset_id)
    if not dataset_dir:
        sys.exit(1)

    script_dir     = Path(__file__).parent
    out_prefix     = script_dir / f"out_d{dataset_id}"
    marginal_file  = str(out_prefix) + "_marginal.txt"
    gif_path       = str(out_prefix) + "_anim.gif"

    print(f"Loading dataset {dataset_id}...")
    grids = load_all_grids(dataset_dir)
    if not grids:
        sys.exit(1)
    print(f"Loaded {len(grids)} observation grids")

    if not os.path.exists(marginal_file):
        print("Error: Output file not found")
        print(f"  Marginal: {os.path.basename(marginal_file)} "
              f"(exists: {os.path.exists(marginal_file)})")
        print("\nRun 'make run' or build the executable first")
        sys.exit(1)

    print("Loading trajectory...")
    marginal_traj = load_trajectory_file(marginal_file)
    print(f"Loaded marginal trajectory with {len(marginal_traj)} points")

    ground_truth = None
    gt_file = find_ground_truth_file(dataset_id)
    if gt_file:
        ground_truth = load_trajectory_file(gt_file)
        if ground_truth:
            print(f"Loaded ground truth trajectory with {len(ground_truth)} points")

    # Always save GIF
    if MATPLOTLIB_AVAILABLE:
        save_gif(grids, marginal_traj, gif_path, ground_truth, fps=1.5)
    else:
        print("Cannot save GIF: matplotlib/numpy not available.")

    # Interactive window
    if MATPLOTLIB_AVAILABLE:
        print("\nLaunching interactive visualization window...\n")
        play_visualization(grids, marginal_traj, ground_truth)
    else:
        print("\nMatplotlib not available. Showing ASCII visualization:")
        for t in range(min(5, len(grids))):
            _ascii_frame(grids, marginal_traj, t, ground_truth)


if __name__ == "__main__":
    main()
