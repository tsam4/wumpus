#!/usr/bin/env python3
"""
Wumpus Trajectory Visualizer

Displays measurements (detections) and predicted Wumpus location at each timestep.
Shows both marginal (per-timestep argmax) and MAP (Viterbi) trajectories.

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
    from matplotlib.animation import FuncAnimation
    MATPLOTLIB_AVAILABLE = NP_AVAILABLE
except ImportError:
    MATPLOTLIB_AVAILABLE = False

try:
    from PIL import Image, ImageDraw, ImageFont
    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False


# ---------------------------------------------------------------------------
# Colour palette  (all solid, no symbols)
# ---------------------------------------------------------------------------
_C_EMPTY      = "#f0f0f0"   # empty, undetected cell
_C_DETECTION  = "#4a90d9"   # sensor detected something here  (blue)
_C_TRUE       = "#e8a020"   # ground-truth wumpus location    (amber)
_C_MARGINAL   = "#2ca02c"   # marginal-argmax prediction      (green)
_C_MAP        = "#d62728"   # MAP / Viterbi prediction        (red)
_C_GRID       = "#cccccc"   # grid line colour


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
    """Load a trajectory file (marginal or MAP)."""
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
    else:
        print(f"Error: Dataset {dataset_id} not found at {dataset_dir.name}")
        return None


def find_ground_truth_file(dataset_id):
    script_dir = Path(__file__).parent
    ground_truth_path = script_dir / "Ground truth for dataset1-20260506" / "wumpus_trajectory.txt"
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
            grid = load_grid_file(filepath)
            grids.append(grid)
        except Exception as e:
            print(f"Error loading {filepath}: {e}")
            return None

    return grids


# ---------------------------------------------------------------------------
# Core colour-block rendering
# ---------------------------------------------------------------------------

def _cell_colour(obs_val, is_true, is_marginal, is_map):
    """
    Return the fill colour for one grid cell.
    Priority (highest → lowest): true > marginal > MAP > detection > empty.
    """
    if is_true:
        return _C_TRUE
    if is_marginal:
        return _C_MARGINAL
    if is_map:
        return _C_MAP
    if obs_val:
        return _C_DETECTION
    return _C_EMPTY


def _build_colour_image(obs_grid, rows, cols, mx, my, vx, vy, tx, ty):
    """
    Build an (rows × cols × 3) uint8 RGB array where every cell is a solid colour.
    """
    from matplotlib.colors import to_rgb
    img = np.zeros((rows, cols, 3), dtype=np.float32)
    for y in range(rows):
        for x in range(cols):
            obs_val  = grid_value(obs_grid, y, x)
            is_true  = (x == tx and y == ty)
            is_marg  = (x == mx and y == my)
            is_map_  = (x == vx and y == vy)
            hex_col  = _cell_colour(obs_val, is_true, is_marg, is_map_)
            img[y, x] = to_rgb(hex_col)
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
    ax.set_title(f"Observations  —  t = {timestep}",
                 fontsize=11, fontweight="bold", pad=10)
    for spine in ax.spines.values():
        spine.set_edgecolor(_C_GRID)


def _legend_patches():
    """Fixed legend showing all four cell types."""
    return [
        mpatches.Patch(color=_C_DETECTION, label="Detection"),
        mpatches.Patch(color=_C_TRUE,      label="True"),
        mpatches.Patch(color=_C_MARGINAL,  label="Marginal"),
        mpatches.Patch(color=_C_MAP,       label="MAP (Viterbi)"),
    ]


# ---------------------------------------------------------------------------
# Static single-frame render  (used by visualize_step)
# ---------------------------------------------------------------------------

def _render_frame(obs_grid, rows, cols, mx, my, vx, vy, tx, ty, timestep,
                  output_path=None):
    cell_px = 60
    dpi = 100
    fig_w = max(5.0, cols * cell_px / dpi + 1.6)
    fig_h = max(4.5, rows * cell_px / dpi + 1.4)

    fig, ax = plt.subplots(figsize=(fig_w, fig_h))
    fig.patch.set_facecolor("white")

    img = _build_colour_image(obs_grid, rows, cols, mx, my, vx, vy, tx, ty)
    ax.imshow(img, origin="upper", interpolation="nearest",
              extent=(-0.5, cols - 0.5, rows - 0.5, -0.5), zorder=1)

    _draw_grid_lines(ax, rows, cols)
    _style_ax(ax, rows, cols, timestep)

    ax.legend(handles=_legend_patches(),
              loc="upper right", fontsize=8,
              framealpha=0.92, edgecolor=_C_GRID)

    fig.tight_layout(pad=1.2)

    if output_path:
        fig.savefig(output_path, dpi=dpi, bbox_inches="tight",
                    facecolor="white")
        print(f"Saved visualization to {output_path}")
    else:
        plt.show()

    plt.close(fig)


# ---------------------------------------------------------------------------
# Public API kept from original  (visualize_step)
# ---------------------------------------------------------------------------

def visualize_step(grids, marginal_trajectory, map_trajectory, timestep,
                   output_path=None, ground_truth=None):
    """Visualize a single timestep with observations and predictions."""
    if timestep >= len(grids):
        print(f"Error: Timestep {timestep} out of range (max {len(grids)-1})")
        return

    obs_grid = grids[timestep]
    rows = len(obs_grid)
    cols = len(obs_grid[0]) if rows else 0

    mx, my = marginal_trajectory[timestep] if timestep < len(marginal_trajectory) else (-1, -1)
    vx, vy = map_trajectory[timestep]      if timestep < len(map_trajectory)      else (-1, -1)
    tx, ty = ground_truth[timestep]        if ground_truth and timestep < len(ground_truth) else (-1, -1)

    if MATPLOTLIB_AVAILABLE:
        _render_frame(obs_grid, rows, cols, mx, my, vx, vy, tx, ty,
                      timestep, output_path)
    else:
        # ASCII fallback
        print(f"\n{'='*60}")
        print(f"Timestep {timestep}")
        print(f"{'='*60}")
        for y in range(rows):
            row_str = ""
            for x in range(cols):
                obs_val = grid_value(obs_grid, y, x)
                c = _cell_colour(obs_val,
                                 x == tx and y == ty,
                                 x == mx and y == my,
                                 x == vx and y == vy)
                row_str += {"#f0f0f0": ".", "#4a90d9": "D",
                            "#e8a020": "T", "#2ca02c": "G",
                            "#d62728": "R"}.get(c, "?")
            print(row_str)
        print(f"Marginal: ({mx},{my})  MAP: ({vx},{vy})"
              + (f"  True: ({tx},{ty})" if tx >= 0 else ""))


# ---------------------------------------------------------------------------
# Animation
# ---------------------------------------------------------------------------

def _create_animation_axes(rows, cols):
    cell_px = 60
    dpi = 100
    fig_w = max(5.0, cols * cell_px / dpi + 1.6)
    fig_h = max(4.5, rows * cell_px / dpi + 1.6)

    fig, ax = plt.subplots(figsize=(fig_w, fig_h))
    fig.patch.set_facecolor("white")

    # Blank RGB image to start
    blank = np.ones((rows, cols, 3), dtype=np.float32)
    obs_img = ax.imshow(blank, origin="upper", interpolation="nearest",
                        extent=(-0.5, cols - 0.5, rows - 0.5, -0.5), zorder=1)

    _draw_grid_lines(ax, rows, cols)
    _style_ax(ax, rows, cols, 0)

    ax.legend(handles=_legend_patches(),
              loc="upper right", fontsize=8,
              framealpha=0.92, edgecolor=_C_GRID)

    fig.tight_layout(pad=1.2)
    return fig, ax, obs_img


def _update_animation_frame(frame, grids, marginal_trajectory, map_trajectory,
                             ground_truth, obs_img, ax):
    obs_grid = grids[frame]
    rows, cols = obs_grid.shape

    mx, my = marginal_trajectory[frame] if frame < len(marginal_trajectory) else (-1, -1)
    vx, vy = map_trajectory[frame]      if frame < len(map_trajectory)      else (-1, -1)
    tx, ty = ground_truth[frame]        if ground_truth and frame < len(ground_truth) else (-1, -1)

    img = _build_colour_image(obs_grid, rows, cols, mx, my, vx, vy, tx, ty)
    obs_img.set_data(img)

    ax.set_title(f"Observations  —  t = {frame}",
                 fontsize=11, fontweight="bold", pad=10)

    return [obs_img]


def play_visualization(grids, marginal_trajectory, map_trajectory,
                       ground_truth=None):
    rows, cols = grids[0].shape
    fig, ax, obs_img = _create_animation_axes(rows, cols)

    FuncAnimation(
        fig,
        _update_animation_frame,
        frames=len(grids),
        fargs=(grids, marginal_trajectory, map_trajectory, ground_truth,
               obs_img, ax),
        interval=800,
        blit=False,
        repeat=False,
    )

    plt.show()


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    """Main function."""
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

    script_dir = Path(__file__).parent
    out_prefix = script_dir / f"out_d{dataset_id}"

    print(f"Loading dataset {dataset_id}...")
    grids = load_all_grids(dataset_dir)
    if not grids:
        sys.exit(1)
    print(f"Loaded {len(grids)} observation grids")

    marginal_file = str(out_prefix) + "_marginal.txt"
    map_file      = str(out_prefix) + "_map.txt"

    if not os.path.exists(marginal_file) or not os.path.exists(map_file):
        print("Error: Output files not found")
        print(f"  Marginal: {os.path.basename(marginal_file)} "
              f"(exists: {os.path.exists(marginal_file)})")
        print(f"  MAP:      {os.path.basename(map_file)} "
              f"(exists: {os.path.exists(map_file)})")
        print("\nRun 'make run' or build the executable first")
        sys.exit(1)

    print("Loading trajectories...")
    marginal_traj = load_trajectory_file(marginal_file)
    map_traj      = load_trajectory_file(map_file)

    ground_truth = None
    gt_file = find_ground_truth_file(dataset_id)
    if gt_file:
        ground_truth = load_trajectory_file(gt_file)
        if ground_truth:
            print(f"Loaded ground truth trajectory with {len(ground_truth)} points")

    print(f"Loaded marginal trajectory with {len(marginal_traj)} points")
    print(f"Loaded MAP trajectory with {len(map_traj)} points")

    if MATPLOTLIB_AVAILABLE:
        print("\nLaunching interactive visualization window...\n")
        play_visualization(grids, marginal_traj, map_traj, ground_truth)
    else:
        print("\nMatplotlib not available. Showing ASCII visualization:")
        for t in range(min(5, len(grids))):
            visualize_step(grids, marginal_traj, map_traj, t)


if __name__ == "__main__":
    main()
