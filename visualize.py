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

# Try to import numpy for array handling
try:
    import numpy as np
    NP_AVAILABLE = True
except ImportError:
    NP_AVAILABLE = False

# Try to import matplotlib for visualization
try:
    import matplotlib
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    from matplotlib.animation import FuncAnimation
    from matplotlib.colors import ListedColormap
    MATPLOTLIB_AVAILABLE = NP_AVAILABLE
except ImportError:
    MATPLOTLIB_AVAILABLE = False

# Try to import PIL for image output
try:
    from PIL import Image, ImageDraw, ImageFont
    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False


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


def visualize_step(grids, marginal_trajectory, map_trajectory, timestep, output_path=None, ground_truth=None):
    """Visualize a single timestep with observations and predictions."""
    
    if timestep >= len(grids):
        print(f"Error: Timestep {timestep} out of range (max {len(grids)-1})")
        return
    
    obs_grid = grids[timestep]
    rows = len(obs_grid)
    cols = len(obs_grid[0]) if rows else 0
    
    # Get predictions at this timestep
    if timestep < len(marginal_trajectory):
        mx, my = marginal_trajectory[timestep]
    else:
        mx, my = -1, -1
    
    if timestep < len(map_trajectory):
        vx, vy = map_trajectory[timestep]
    else:
        vx, vy = -1, -1

    if ground_truth and timestep < len(ground_truth):
        tx, ty = ground_truth[timestep]
    else:
        tx, ty = -1, -1
    
    # Use matplotlib if available
    if MATPLOTLIB_AVAILABLE:
        _render_obs_panel(obs_grid, rows, cols, mx, my, vx, vy, tx, ty, timestep, output_path)
    
    # Fallback: ASCII visualization
    else:
        print(f"\n{'='*60}")
        print(f"Timestep {timestep}")
        print(f"{'='*60}")
        
        # Print observations
        print("\nObservations (M=measurement, T=true, G=marginal, R=MAP, X=overlap):")
        for y in range(rows):
            row_str = ""
            for x in range(cols):
                obs_val = grid_value(obs_grid, y, x)
                is_true = x == tx and y == ty
                is_marginal = x == mx and y == my
                is_map = x == vx and y == vy
                if is_true and (is_marginal or is_map):
                    row_str += "X"
                elif is_true:
                    row_str += "T"
                elif obs_val > 0:
                    if is_marginal:
                        row_str += "G"
                    elif is_map:
                        row_str += "R"
                    else:
                        row_str += "M"
                else:
                    if is_marginal:
                        row_str += "g"
                    elif is_map:
                        row_str += "r"
                    else:
                        row_str += "."
            print(row_str)
        
        if 0 <= tx < cols and 0 <= ty < rows:
            print(f"\nTrue location: ({tx}, {ty})")
        print(f"Marginal prediction: ({mx}, {my})")
        print(f"MAP (Viterbi) prediction: ({vx}, {vy})")


# ---------------------------------------------------------------------------
# Single-panel rendering helpers
# ---------------------------------------------------------------------------

# Colours used throughout the observation panel
_CELL_BG      = "#f5f5f5"   # empty cell
_CELL_DET     = "#4a90d9"   # detected cell (light blue)
_GRID_LINE    = "#cccccc"
_COL_MARGINAL = "#2ca02c"   # green
_COL_MAP      = "#d62728"   # red
_COL_TRUE     = "#1a1a1a"   # near-black


def _make_obs_cmap():
    """Two-colour map: light grey → steel blue."""
    return ListedColormap([_CELL_BG, _CELL_DET])


def _draw_cell_grid(ax, rows, cols):
    """Draw thin grid lines on top of the image."""
    for x in range(cols + 1):
        ax.axvline(x - 0.5, color=_GRID_LINE, linewidth=0.6, zorder=2)
    for y in range(rows + 1):
        ax.axhline(y - 0.5, color=_GRID_LINE, linewidth=0.6, zorder=2)


def _style_obs_ax(ax, rows, cols, timestep):
    """Apply consistent styling to the observation axes."""
    ax.set_facecolor("white")
    ax.set_xlim(-0.5, cols - 0.5)
    ax.set_ylim(rows - 0.5, -0.5)
    ax.set_xticks(range(cols))
    ax.set_yticks(range(rows))
    ax.tick_params(labelsize=8, length=0)
    ax.set_xlabel("Column (x)", fontsize=9, labelpad=6)
    ax.set_ylabel("Row (y)", fontsize=9, labelpad=6)
    ax.set_title(f"Observations  —  t = {timestep}", fontsize=11, fontweight="bold", pad=10)
    for spine in ax.spines.values():
        spine.set_edgecolor(_GRID_LINE)


def _render_obs_panel(obs_grid, rows, cols, mx, my, vx, vy, tx, ty, timestep, output_path=None):
    """Render the single observation panel and either show or save it."""
    # Scale figure so each cell is ~55 px at 100 dpi
    cell_px = 55
    dpi = 100
    fig_w = max(5.0, cols * cell_px / dpi + 1.6)
    fig_h = max(4.0, rows * cell_px / dpi + 1.4)

    fig, ax = plt.subplots(figsize=(fig_w, fig_h))
    fig.patch.set_facecolor("white")

    # Background image
    cmap = _make_obs_cmap()
    ax.imshow(obs_grid, cmap=cmap, origin="upper", vmin=0, vmax=1,
              interpolation="nearest", zorder=1)

    # Grid lines
    _draw_cell_grid(ax, rows, cols)

    # --- markers (drawn in order: detection dots → true → marginal → MAP) ---

    # Blue dot in every detected cell
    det_ys, det_xs = np.where(obs_grid > 0)
    if len(det_xs):
        ax.scatter(det_xs, det_ys,
                   c="#1565c0", marker="o", s=40, zorder=4,
                   label="Detection")

    # True position  ✕
    if 0 <= tx < cols and 0 <= ty < rows:
        ax.plot(tx, ty, marker="x", color=_COL_TRUE,
                markersize=14, markeredgewidth=2.5, zorder=6,
                linestyle="none", label="True")

    # Marginal argmax  ○
    if 0 <= mx < cols and 0 <= my < rows:
        ax.plot(mx, my, marker="o", color=_COL_MARGINAL,
                markersize=16, markerfacecolor="none",
                markeredgewidth=2.2, zorder=7, linestyle="none",
                label="Marginal")

    # MAP (Viterbi)  △
    if 0 <= vx < cols and 0 <= vy < rows:
        ax.plot(vx, vy, marker="^", color=_COL_MAP,
                markersize=14, markerfacecolor="none",
                markeredgewidth=2.2, zorder=8, linestyle="none",
                label="MAP (Viterbi)")

    _style_obs_ax(ax, rows, cols, timestep)

    # Legend — only show entries that actually appear
    handles, labels = ax.get_legend_handles_labels()
    if handles:
        ax.legend(handles, labels,
                  loc="upper right",
                  fontsize=8,
                  framealpha=0.9,
                  edgecolor=_GRID_LINE,
                  markerscale=0.85)

    fig.tight_layout(pad=1.2)

    if output_path:
        fig.savefig(output_path, dpi=dpi, bbox_inches="tight",
                    facecolor="white")
        print(f"Saved visualization to {output_path}")
    else:
        plt.show()

    plt.close(fig)


# ---------------------------------------------------------------------------
# Animation helpers
# ---------------------------------------------------------------------------

def _create_animation_axes(rows, cols):
    """Create figure + axes for the animated playback (single panel)."""
    cell_px = 55
    dpi = 100
    fig_w = max(5.0, cols * cell_px / dpi + 1.6)
    fig_h = max(4.5, rows * cell_px / dpi + 1.6)

    fig, ax = plt.subplots(figsize=(fig_w, fig_h))
    fig.patch.set_facecolor("white")

    cmap = _make_obs_cmap()
    obs_img = ax.imshow(np.zeros((rows, cols), dtype=int),
                        cmap=cmap, origin="upper",
                        vmin=0, vmax=1,
                        interpolation="nearest", zorder=1)

    _draw_cell_grid(ax, rows, cols)
    _style_obs_ax(ax, rows, cols, 0)

    scat_meas = ax.scatter([], [], c="#1565c0", marker="o", s=40,
                           zorder=4, label="Detection")
    pt_true,    = ax.plot([], [], marker="x",  color=_COL_TRUE,
                          markersize=14, markeredgewidth=2.5,
                          linestyle="none", zorder=6, label="True")
    pt_marg,    = ax.plot([], [], marker="o",  color=_COL_MARGINAL,
                          markersize=16, markerfacecolor="none",
                          markeredgewidth=2.2, linestyle="none",
                          zorder=7, label="Marginal")
    pt_map,     = ax.plot([], [], marker="^",  color=_COL_MAP,
                          markersize=14, markerfacecolor="none",
                          markeredgewidth=2.2, linestyle="none",
                          zorder=8, label="MAP (Viterbi)")

    ax.legend(loc="upper right", fontsize=8,
              framealpha=0.9, edgecolor=_GRID_LINE, markerscale=0.85)

    fig.tight_layout(pad=1.2)

    return fig, ax, obs_img, scat_meas, pt_true, pt_marg, pt_map


def _update_animation_frame(frame, grids, marginal_trajectory, map_trajectory,
                             ground_truth, obs_img, scat_meas,
                             pt_true, pt_marg, pt_map, ax):
    obs_grid = grids[frame]
    rows, cols = obs_grid.shape

    mx, my = marginal_trajectory[frame] if frame < len(marginal_trajectory) else (-1, -1)
    vx, vy = map_trajectory[frame]      if frame < len(map_trajectory)      else (-1, -1)
    tx, ty = ground_truth[frame]        if ground_truth and frame < len(ground_truth) else (-1, -1)

    obs_img.set_data(obs_grid)

    det_ys, det_xs = np.where(obs_grid > 0)
    if len(det_xs):
        scat_meas.set_offsets(np.column_stack((det_xs, det_ys)))
    else:
        scat_meas.set_offsets(np.zeros((0, 2)))

    pt_true.set_data([tx] if 0 <= tx < cols and 0 <= ty < rows else [],
                     [ty] if 0 <= tx < cols and 0 <= ty < rows else [])
    pt_marg.set_data([mx] if 0 <= mx < cols and 0 <= my < rows else [],
                     [my] if 0 <= mx < cols and 0 <= my < rows else [])
    pt_map.set_data( [vx] if 0 <= vx < cols and 0 <= vy < rows else [],
                     [vy] if 0 <= vx < cols and 0 <= vy < rows else [])

    ax.set_title(f"Observations  —  t = {frame}",
                 fontsize=11, fontweight="bold", pad=10)

    return [obs_img, scat_meas, pt_true, pt_marg, pt_map]


def play_visualization(grids, marginal_trajectory, map_trajectory, ground_truth=None):
    rows, cols = grids[0].shape
    fig, ax, obs_img, scat_meas, pt_true, pt_marg, pt_map = \
        _create_animation_axes(rows, cols)

    anim = FuncAnimation(
        fig,
        _update_animation_frame,
        frames=len(grids),
        fargs=(grids, marginal_trajectory, map_trajectory, ground_truth,
               obs_img, scat_meas, pt_true, pt_marg, pt_map, ax),
        interval=800,
        blit=False,
        repeat=False,
    )

    plt.show()


def main():
    """Main function."""
    
    # Parse command-line arguments
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
    
    # Find dataset and output files
    dataset_dir = find_dataset_dir(dataset_id)
    if not dataset_dir:
        sys.exit(1)
    
    script_dir = Path(__file__).parent
    out_prefix = script_dir / f"out_d{dataset_id}"
    
    # Load data
    print(f"Loading dataset {dataset_id}...")
    grids = load_all_grids(dataset_dir)
    if not grids:
        sys.exit(1)
    
    print(f"Loaded {len(grids)} observation grids")
    
    # Load trajectories
    marginal_file = str(out_prefix) + "_marginal.txt"
    map_file = str(out_prefix) + "_map.txt"
    
    if not os.path.exists(marginal_file) or not os.path.exists(map_file):
        print(f"Error: Output files not found")
        print(f"  Marginal: {os.path.basename(marginal_file)} (exists: {os.path.exists(marginal_file)})")
        print(f"  MAP:      {os.path.basename(map_file)} (exists: {os.path.exists(map_file)})")
        print("\nRun 'make run' or build the executable first")
        sys.exit(1)
    
    print(f"Loading trajectories...")
    marginal_traj = load_trajectory_file(marginal_file)
    map_traj = load_trajectory_file(map_file)
    
    ground_truth = None
    ground_truth_file = find_ground_truth_file(dataset_id)
    if ground_truth_file:
        ground_truth = load_trajectory_file(ground_truth_file)
        if ground_truth:
            print(f"Loaded ground truth trajectory with {len(ground_truth)} points")

    print(f"Loaded marginal trajectory with {len(marginal_traj)} points")
    print(f"Loaded MAP trajectory with {len(map_traj)} points")
    
    # Generate visualizations
    if MATPLOTLIB_AVAILABLE:
        print(f"\nLaunching interactive visualization window...\n")
        play_visualization(grids, marginal_traj, map_traj, ground_truth)
    else:
        print("\nMatplotlib not available. Showing ASCII visualization for first few timesteps:")
        for t in range(min(5, len(grids))):
            visualize_step(grids, marginal_traj, map_traj, t)


if __name__ == "__main__":
    main()
