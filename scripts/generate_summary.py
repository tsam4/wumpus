#!/usr/bin/env python3
"""
Generate comprehensive summary report for wumpus tracking results.

Displays output files, accuracy metrics, and algorithm details.
"""

import os
import sys
from datetime import datetime

PROJECT_DIR = os.environ.get("PROJECT_DIR", ".")
OUT_D1 = os.environ.get("OUT_D1", f"{PROJECT_DIR}/out_d1")
OUT_D2 = os.environ.get("OUT_D2", f"{PROJECT_DIR}/out_d2")
OUT_D3 = os.environ.get("OUT_D3", f"{PROJECT_DIR}/out_d3")
OUT_D4 = os.environ.get("OUT_D4", f"{PROJECT_DIR}/out_d4")
OUT_D5 = os.environ.get("OUT_D5", f"{PROJECT_DIR}/out_d5")
GROUND_TRUTH = os.environ.get("GROUND_TRUTH", f"{PROJECT_DIR}/Ground truth for dataset1-20260506/wumpus_trajectory.txt")
DATASET_DIR = os.environ.get("DATASET_DIR", f"{PROJECT_DIR}/Datasets-20260506")


def get_file_size(filepath):
    try:
        size = os.path.getsize(filepath)
        return f"{size} bytes"
    except:
        return "N/A"


def count_lines(filepath):
    try:
        with open(filepath, 'r') as f:
            return len([line for line in f if line.strip()])
    except:
        return 0


def read_trajectory(filepath):
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
    except:
        return None


def compute_accuracy(truth, predicted):
    if not truth or not predicted or len(truth) != len(predicted):
        return None
    matches = sum(1 for t, p in zip(truth, predicted) if t == p)
    return 100.0 * matches / len(truth)


def main():
    out_files = [
        ("Dataset1 (5×5, 10 steps)", f"{OUT_D1}_marginal.txt", f"{OUT_D1}_map.txt", GROUND_TRUTH),
        ("Dataset2 (20×20, 20 steps)", f"{OUT_D2}_marginal.txt", f"{OUT_D2}_map.txt", None),
        ("Dataset3 (10×20, 20 steps)", f"{OUT_D3}_marginal.txt", f"{OUT_D3}_map.txt", None),
        ("Dataset4 (4×4, 100 steps)", f"{OUT_D4}_marginal.txt", f"{OUT_D4}_map.txt", None),
        ("Dataset5 (7×7, 200 steps)", f"{OUT_D5}_marginal.txt", f"{OUT_D5}_map.txt", None),
    ]

    print()
    print("\033[1;36m" + "=" * 75 + "\033[0m")
    print("\033[1;36m" + " " * 15 + "WUMPUS TRACKER - COMPREHENSIVE SUMMARY REPORT" + " " * 15 + "\033[0m")
    print("\033[1;36m" + "=" * 75 + "\033[0m")
    print()
    print(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"Project Directory: {PROJECT_DIR}")
    print(f"Dataset Directory: {DATASET_DIR}")
    print()

    print("\033[1;33m" + "=" * 75 + "\033[0m")
    print("\033[1;33mOUTPUT FILES\033[0m")
    print("\033[1;33m" + "=" * 75 + "\033[0m")

    for label, marginal_file, map_file, ground_truth_file in out_files:
        print(f"\n{label}:")

        if os.path.exists(marginal_file):
            lines_m = count_lines(marginal_file)
            size_m = get_file_size(marginal_file)
            print(f"  ✓ Marginal: {marginal_file}")
            print(f"    Size: {size_m}, Lines: {lines_m}")
        else:
            print(f"  ✗ Marginal: {marginal_file} (NOT FOUND)")

        if os.path.exists(map_file):
            lines_map = count_lines(map_file)
            size_map = get_file_size(map_file)
            print(f"  ✓ MAP:      {map_file}")
            print(f"    Size: {size_map}, Lines: {lines_map}")
        else:
            print(f"  ✗ MAP:      {map_file} (NOT FOUND)")

        # Compute accuracy if ground truth available
        if ground_truth_file and os.path.exists(ground_truth_file):
            truth = read_trajectory(ground_truth_file)
            marginal = read_trajectory(marginal_file)
            map_traj = read_trajectory(map_file)

            acc_m = compute_accuracy(truth, marginal)
            acc_map = compute_accuracy(truth, map_traj)

            if acc_m is not None:
                print(f"  ✓ Marginal Accuracy: \033[1;32m{acc_m:.1f}%\033[0m")
            if acc_map is not None:
                print(f"  ✓ MAP Accuracy:      \033[1;32m{acc_map:.1f}%\033[0m")

    print()
    print("\033[1;33m" + "=" * 75 + "\033[0m")
    print("\033[1;33mDATASET CONFIGURATIONS\033[0m")
    print("\033[1;33m" + "=" * 75 + "\033[0m")
    print(
        """
Dataset1: 5×5 grid, 10 timesteps
  - pw = 0.95 (true cell detection)
  - pc = 0.05 (clutter probability)
  - Parameters: KNOWN (fixed)
  - Ground Truth: Available for accuracy comparison

Dataset2: 20×20 grid, 20 timesteps
  - pw = 0.90 (true cell detection)
  - pc = 0.10 (clutter probability)
  - Parameters: KNOWN (fixed)
  - Scale Test: ~20M grid cells, validates performance

Dataset3: 10×20 grid, 20 timesteps
  - pw, pc: UNKNOWN (estimated via EM, 6 iterations)
  - Parameters: Learned from observations
  - Learning: pw/pc estimation

Dataset4: 4×4 grid, 100 timesteps
  - pw = 0.95, pc = 0.05 (fixed)
  - Occupancy: UNKNOWN (estimated via EM, 6 iterations)
  - Learning: Which cells are blocked/occupied

Dataset5: 7×7 grid, 200 timesteps
  - pw, pc, occupancy: ALL UNKNOWN
  - Learning: pw/pc + occupancy via EM (8 iterations)
  - Complexity: Simultaneous parameter and occupancy estimation
"""
    )

    print("\033[1;33m" + "=" * 75 + "\033[0m")
    print("\033[1;33mALGORITHM DETAILS\033[0m")
    print("\033[1;33m" + "=" * 75 + "\033[0m")
    print(
        """
Model Type: Hidden Markov Model (HMM)
  - State space: X_t ∈ {0..rows×cols-1} (discrete grid cells)
  - Observations: Z_t = binary detection grid per timestep
  - Structure: Chain (X₀ → X₁ → X₂ → ...)

Transition Model: Random Walk with Failure
  - 4-neighbor adjacency (up, down, left, right)
  - P(X_t = next | X_{t-1} = prev):
    * 0.25 per direction if open (neighbor reachable)
    * 0.0 if blocked or out-of-bounds
  - Occupancy: P(open | prev) = 1 - occ_prior

Emission Model: Noisy Grid Observations
  - P(Z_t | X_t) = product over all grid cells
  - True cell (s = X_t): p_w (detection) or 1-p_w (miss)
  - Other cells: p_c (clutter) or 1-p_c (clean)
  - Conditional independence: observations independent given state

Inference Algorithm:
  - Marginal: Loopy Belief Propagation (sum-product)
    * Computes P(X_t | Z_1:T) for each timestep
    * Output: argmax of marginals (per-step best guess)
    * Use case: Single-timestep accuracy
  
  - MAP: Viterbi Algorithm (max-product DP)
    * Computes arg max_{X_1:T} P(X_1:T | Z_1:T)
    * Output: Most probable complete trajectory
    * Use case: Joint sequence accuracy
    * Complexity: O(T × S²) where S = # states, T = # timesteps

Learning: Expectation-Maximization (EM)
  - E-step: Run BP to get posteriors P(X_t | Z_1:T)
  - M-step (pw/pc):
    * Expected true detections from posteriors
    * ML estimate: pw = (true dets) / T, pc = (clutter) / (T×(S-1))
  - M-step (occupancy):
    * Count expected attempts and successes per cell
    * Beta-prior: P(open | attempts, successes) = Beta posterior
    * occ = 1 - P(open)
"""
    )

    print("\033[1;36m" + "=" * 75 + "\033[0m")
    print("\033[1;36mEND OF REPORT" + " " * 56 + "\033[0m")
    print("\033[1;36m" + "=" * 75 + "\033[0m\n")


if __name__ == "__main__":
    main()
