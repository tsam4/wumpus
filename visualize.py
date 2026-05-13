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
    from matplotlib.animation import FuncAnimation
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
        fig, axes = plt.subplots(1, 2, figsize=(12, 5))
        
        # Plot observations
        ax = axes[0]
        ax.imshow(obs_grid, cmap='RdYlBu_r', origin='upper', vmin=0, vmax=1)
        ax.set_title(f"Observations (t={timestep})")
        ax.set_xlabel("X (column)")
        ax.set_ylabel("Y (row)")
        
        # Mark detections
        for y in range(rows):
            for x in range(cols):
                if grid_value(obs_grid, y, x) > 0:
                    ax.plot(x, y, 'b*', markersize=15, label="Detection" if x == 0 and y == 0 else "")
        
        # Mark marginal prediction
        if 0 <= mx < cols and 0 <= my < rows:
            ax.plot(mx, my, 'go', markersize=12, markerfacecolor='none', linewidth=2, label="Marginal")
        
        # Mark MAP prediction
        if 0 <= vx < cols and 0 <= vy < rows:
            ax.plot(vx, vy, 'r^', markersize=12, markerfacecolor='none', linewidth=2, label="MAP (Viterbi)")

        # Mark true location if available
        if 0 <= tx < cols and 0 <= ty < rows:
            ax.plot(tx, ty, 'kx', markersize=12, markeredgewidth=2, label="True")
        
        ax.legend(loc='upper right')
        
        # Plot full trajectories
        ax = axes[1]
        
        # Draw grid background
        ax.imshow(np.zeros((rows, cols)), cmap='gray', origin='upper', alpha=0.3)
        
        # Plot marginal trajectory
        if marginal_trajectory:
            mx_all = [p[0] for p in marginal_trajectory]
            my_all = [p[1] for p in marginal_trajectory]
            ax.plot(mx_all, my_all, 'g-', linewidth=2, alpha=0.7, label="Marginal trajectory")
            ax.plot(mx_all, my_all, 'go', markersize=6, alpha=0.5)
        
        # Plot MAP trajectory
        if map_trajectory:
            vx_all = [p[0] for p in map_trajectory]
            vy_all = [p[1] for p in map_trajectory]
            ax.plot(vx_all, vy_all, 'r--', linewidth=2, alpha=0.7, label="MAP trajectory")
            ax.plot(vx_all, vy_all, 'r^', markersize=6, alpha=0.5)

        # Plot true trajectory if available
        if ground_truth:
            tx_all = [p[0] for p in ground_truth]
            ty_all = [p[1] for p in ground_truth]
            ax.plot(tx_all, ty_all, 'k-', linewidth=2, alpha=0.7, label="True trajectory")
            ax.plot(tx_all, ty_all, 'kx', markersize=6, alpha=0.5)
        
        # Mark current timestep
        if timestep < len(marginal_trajectory):
            ax.plot(mx, my, 'go', markersize=15, markerfacecolor='none', linewidth=2)
        
        ax.set_xlim(-0.5, cols - 0.5)
        ax.set_ylim(rows - 0.5, -0.5)
        ax.set_xlabel("X (column)")
        ax.set_ylabel("Y (row)")
        ax.set_title(f"Trajectories (current: t={timestep})")
        ax.legend(loc='upper right')
        ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        if output_path:
            plt.savefig(output_path, dpi=100, bbox_inches='tight')
            print(f"Saved visualization to {output_path}")
        else:
            plt.show()
        
        plt.close()
    
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


def create_visualization_axes(rows, cols):
    fig, (ax_obs, ax_traj) = plt.subplots(1, 2, figsize=(14, 6))
    obs_img = ax_obs.imshow(np.zeros((rows, cols)), cmap='RdYlBu_r', origin='upper', vmin=0, vmax=1)
    ax_obs.set_facecolor('#fafafa')
    ax_obs.set_title("Observations")
    ax_obs.set_xlabel("X (column)")
    ax_obs.set_ylabel("Y (row)")
    ax_obs.set_xticks(range(cols))
    ax_obs.set_yticks(range(rows))
    ax_obs.set_xlim(-0.5, cols - 0.5)
    ax_obs.set_ylim(rows - 0.5, -0.5)
    ax_obs.grid(False)

    scat_meas = ax_obs.scatter([], [], c='b', marker='*', s=120, label='Measurement')
    scat_marg = ax_obs.scatter([], [], edgecolors='g', facecolors='none', marker='o', s=140, linewidths=2, label='Marginal')
    scat_map = ax_obs.scatter([], [], edgecolors='r', facecolors='none', marker='^', s=140, linewidths=2, label='MAP')
    scat_true = ax_obs.scatter([], [], c='k', marker='x', s=140, linewidths=2, label='True')
    ax_obs.legend(loc='upper right', fontsize='small')

    ax_traj.imshow(np.zeros((rows, cols)), cmap='gray', origin='upper', alpha=0.12, vmin=0, vmax=1)
    ax_traj.set_facecolor('#f0f0f0')
    ax_traj.set_title("Trajectories")
    ax_traj.set_xlabel("X (column)")
    ax_traj.set_ylabel("Y (row)")
    ax_traj.set_xticks(range(cols))
    ax_traj.set_yticks(range(rows))
    ax_traj.set_xlim(-0.5, cols - 0.5)
    ax_traj.set_ylim(rows - 0.5, -0.5)
    ax_traj.grid(True, alpha=0.3)

    line_marg, = ax_traj.plot([], [], 'g-o', linewidth=2, alpha=0.7, label='Marginal')
    line_map, = ax_traj.plot([], [], 'r--^', linewidth=2, alpha=0.7, label='MAP')
    line_true, = ax_traj.plot([], [], 'k-x', linewidth=2, alpha=0.7, label='True')
    current_point, = ax_traj.plot([], [], 'yo', markersize=10, markerfacecolor='none', label='Current')
    ax_traj.legend(loc='upper right', fontsize='small')

    return fig, ax_obs, ax_traj, obs_img, scat_meas, scat_marg, scat_map, scat_true, line_marg, line_map, line_true, current_point


def update_animation_frame(frame, grids, marginal_trajectory, map_trajectory, ground_truth, obs_img, scat_meas, scat_marg, scat_map, scat_true, line_marg, line_map, line_true, current_point, ax_obs, ax_traj):
    obs_grid = grids[frame]
    rows = len(obs_grid)
    cols = len(obs_grid[0]) if rows else 0

    if frame < len(marginal_trajectory):
        mx, my = marginal_trajectory[frame]
    else:
        mx, my = -1, -1

    if frame < len(map_trajectory):
        vx, vy = map_trajectory[frame]
    else:
        vx, vy = -1, -1

    if ground_truth and frame < len(ground_truth):
        tx, ty = ground_truth[frame]
    else:
        tx, ty = -1, -1

    obs_img.set_data(obs_grid)

    meas_y, meas_x = np.where(obs_grid > 0)
    if len(meas_x):
        scat_meas.set_offsets(np.column_stack((meas_x, meas_y)))
    else:
        scat_meas.set_offsets(np.zeros((0, 2)))

    if 0 <= mx < cols and 0 <= my < rows:
        scat_marg.set_offsets(np.array([[mx, my]]))
    else:
        scat_marg.set_offsets(np.zeros((0, 2)))

    if 0 <= vx < cols and 0 <= vy < rows:
        scat_map.set_offsets(np.array([[vx, vy]]))
    else:
        scat_map.set_offsets(np.zeros((0, 2)))

    if 0 <= tx < cols and 0 <= ty < rows:
        scat_true.set_offsets(np.array([[tx, ty]]))
    else:
        scat_true.set_offsets(np.zeros((0, 2)))

    mx_all = [p[0] for p in marginal_trajectory]
    my_all = [p[1] for p in marginal_trajectory]
    line_marg.set_data(mx_all, my_all)

    vx_all = [p[0] for p in map_trajectory]
    vy_all = [p[1] for p in map_trajectory]
    line_map.set_data(vx_all, vy_all)

    if ground_truth:
        tx_all = [p[0] for p in ground_truth]
        ty_all = [p[1] for p in ground_truth]
        line_true.set_data(tx_all, ty_all)
    else:
        line_true.set_data([], [])

    if 0 <= mx < cols and 0 <= my < rows:
        current_point.set_data([mx], [my])
    else:
        current_point.set_data([], [])

    ax_obs.set_title(f"Observations (t={frame})")
    ax_traj.set_title(f"Trajectories (current: t={frame})")

    return [obs_img, scat_meas, scat_marg, scat_map, scat_true, line_marg, line_map, line_true, current_point]


def play_visualization(grids, marginal_trajectory, map_trajectory, ground_truth=None):
    rows = len(grids[0])
    cols = len(grids[0][0])
    fig, ax_obs, ax_traj, obs_img, scat_meas, scat_marg, scat_map, scat_true, line_marg, line_map, line_true, current_point = create_visualization_axes(rows, cols)

    anim = FuncAnimation(
        fig,
        update_animation_frame,
        frames=len(grids),
        fargs=(grids, marginal_trajectory, map_trajectory, ground_truth, obs_img, scat_meas, scat_marg, scat_map, scat_true, line_marg, line_map, line_true, current_point, ax_obs, ax_traj),
        interval=800,
        blit=False,
        repeat=False
    )

    fig.suptitle("Wumpus trajectory playback")
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
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
