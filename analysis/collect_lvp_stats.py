import shutil
import subprocess
import pathlib
import time
from typing import List

from m5stats import M5Stats

import charts

M5_DIRPATH = pathlib.Path("../m5out")

STATS_DIRPATH = pathlib.Path("stats")
OUTPUT_DIRPATH = pathlib.Path("charts")

LVPSTATS_PATH = "system.cpu.lvp"

BENCHMARK_NAMES = ("sjeng", "leslie3d", "lbm", "astar", "milc", "namd")

STATSFILE_SUFFIX = "_stats.txt"


def run_benchmark(benchmark_name: str):
    # Call the makefile with this benchmark name
    run_result = subprocess.run([
        "make", f"{benchmark_name}"],
        cwd=".."
    )

    # Copy the stats over
    shutil.copy2(
        M5_DIRPATH/"stats.txt",
        STATS_DIRPATH/(benchmark_name + STATSFILE_SUFFIX)
    )


def lvp_chart(benchmark_name: str, out_filepath: str):
    benchmark_path = STATS_DIRPATH / (benchmark_name + STATSFILE_SUFFIX)
    
    stats = M5Stats(benchmark_path)
    lvp_stats = stats.get_stats(LVPSTATS_PATH).as_dict()

    charts.bar_chart(
        f"LVP stats for {benchmark_name}",
        "Statistic",
        "Value",
        lvp_stats.keys(),
        lvp_stats.values(),
        out_filepath,
    )


def main():
    program_start = time.perf_counter()

    for benchmark_name in BENCHMARK_NAMES:
        start = time.perf_counter()

        print(f"Running benchmark {benchmark_name}")
        run_benchmark(benchmark_name)

        print(f"Generating chart for benchmark {benchmark_name}")
        output_path = OUTPUT_DIRPATH / f"{benchmark_name}_lvp.png"
        lvp_chart(benchmark_name, output_path)

        end = time.perf_counter()
        print(f"Done in {(end - start)*1000:.3f} ms")

    program_end = time.perf_counter()
    print(f"Finished in {(program_end - program_start)*1000:.3f} ms")


if __name__ == "__main__":
    main()
