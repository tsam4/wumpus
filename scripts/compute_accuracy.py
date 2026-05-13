#!/usr/bin/env python3
"""
Compute accuracy of predicted trajectories vs ground truth.

Usage: python3 compute_accuracy.py <ground_truth> <marginal_output> <map_output>

Outputs per-timestep accuracy and statistics for both marginal and MAP trajectories.
"""

import sys
import os


def read_trajectory(filepath):
    """Read trajectory file: each line is 'x y' (space-separated)"""
    try:
        with open(filepath, 'r') as f:
            trajectory = []
            for line in f:
                line = line.strip()
                if not line:
                    continue
                parts = line.split()
                if len(parts) >= 2:
                    trajectory.append((int(parts[0]), int(parts[1])))
            return trajectory
    except Exception as e:
        print(f"Error reading {filepath}: {e}", file=sys.stderr)
        return None


def compute_accuracy(truth, predicted):
    """Compute per-timestep accuracy (% of matching positions)"""
    if len(truth) != len(predicted):
        print(
            f"Warning: trajectory lengths differ (truth={len(truth)}, predicted={len(predicted)})",
            file=sys.stderr,
        )
        min_len = min(len(truth), len(predicted))
        truth = truth[:min_len]
        predicted = predicted[:min_len]

    matches = sum(1 for t, p in zip(truth, predicted) if t == p)
    accuracy = 100.0 * matches / len(truth) if truth else 0.0
    return accuracy, matches, len(truth)


def main():
    if len(sys.argv) < 4:
        print("Usage: compute_accuracy.py <ground_truth> <marginal_output> <map_output>")
        sys.exit(1)

    ground_truth_file = sys.argv[1]
    marginal_file = sys.argv[2]
    map_file = sys.argv[3]

    print("\033[36m" + "=" * 70 + "\033[0m")

    ground_truth = read_trajectory(ground_truth_file)
    marginal = read_trajectory(marginal_file)
    map_traj = read_trajectory(map_file)

    print("\033[36m" + "=" * 70 + "\033[0m")

    if not ground_truth:
        print("\033[0;31m✗ Failed to load ground truth\033[0m")
        sys.exit(1)

    print(f"\n\033[1;33m[Ground Truth]\033[0m")
    print(f"  Length: {len(ground_truth)} timesteps")
    print(f"  First 5 steps: {ground_truth[:5]}")
    print(f"  Last 5 steps:  {ground_truth[-5:]}")

    if marginal:
        acc_m, matches_m, total_m = compute_accuracy(ground_truth, marginal)
        print(f"\n\033[1;33m[Marginal Trajectory]\033[0m")
        print(f"  Length: {len(marginal)} timesteps")
        print(f"  Accuracy: \033[1;32m{acc_m:.1f}%\033[0m ({matches_m}/{total_m} steps correct)")
        print(f"  First 5 steps: {marginal[:5]}")
        print(f"  Last 5 steps:  {marginal[-5:]}")
    else:
        print(f"\n\033[0;31m✗ Failed to load marginal trajectory\033[0m")

    if map_traj:
        acc_m, matches_m, total_m = compute_accuracy(ground_truth, map_traj)
        print(f"\n\033[1;33m[MAP Trajectory]\033[0m")
        print(f"  Length: {len(map_traj)} timesteps")
        print(f"  Accuracy: \033[1;32m{acc_m:.1f}%\033[0m ({matches_m}/{total_m} steps correct)")
        print(f"  First 5 steps: {map_traj[:5]}")
        print(f"  Last 5 steps:  {map_traj[-5:]}")
    else:
        print(f"\n\033[0;31m✗ Failed to load MAP trajectory\033[0m")

    print("\n\033[36m" + "=" * 70 + "\033[0m\n")


if __name__ == "__main__":
    main()
