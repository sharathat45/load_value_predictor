#!/bin/bash

# Define the benchmarks and prediction degrade values
BENCHMARKS=("sjeng" "leslie3d" "lbm" "astar" "milc" "namd")
PREDICTION_DEGRADES=(0)

# Create an empty output file
mkdir -p "analysis/execresults"
echo -n > "analysis/execresults/out.txt"

# Loop over the benchmarks and prediction degrade values
for BENCHMARK in "${BENCHMARKS[@]}"; do
  for PREDICTION_DEGRADE in "${PREDICTION_DEGRADES[@]}"; do
    # Run the command with the current benchmark and prediction degrade value
    ./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py --cpu-type=O3CPU --maxinsts=1000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache --enable-lvp -b "$BENCHMARK"
    # Extract the IPC value from the stats file
    IPC=$(grep "system.cpu.ipc" m5out/stats.txt | awk '{print $2}')

    # Copy the resulting stats.txt file to the appropriate directory
    mkdir -p "analysis/execresults/$BENCHMARK"
    cp m5out/stats.txt "analysis/execresults/$BENCHMARK/stats_$PREDICTION_DEGRADE.txt"

    # Append the IPC value to the output file
    echo "$BENCHMARK $PREDICTION_DEGRADE $IPC" >> "part1/execresults/out.txt"
  done
done