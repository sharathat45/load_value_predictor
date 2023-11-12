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

INSTRMIX_PATH = "system.cpu.exec_context.thread_0.statExecutedInstType"

BENCHMARK_NAMES = ("sjeng", "leslie3d", "lbm", "astar", "milc", "namd")

STATSFILE_SUFFIX = "_stats.txt"


def run_benchmark(benchmark_name: str):
    # Call the makefile with this benchmark name
    run_result = subprocess.run([
        "make", "benchmark", f"BENCHMARK_BIN={benchmark_name}"],
        cwd=".."
    )

    # Copy the stats over
    shutil.copy2(
        M5_DIRPATH/"stats.txt",
        STATS_DIRPATH/(benchmark_name + STATSFILE_SUFFIX)
    )


def overall_ipc(benchmark_path: str) -> float:
    stats = M5Stats(benchmark_path)
    insts = stats.stats["simInsts"]
    cycles = stats.get_stats("system.cpu")["numCycles"]
    return insts.value / cycles.value


def ipc_chart(benchmark_names: List[str], out_filepath: str):
    benchmark_paths = [
        STATS_DIRPATH / (benchmark_name + STATSFILE_SUFFIX)
        for benchmark_name in BENCHMARK_NAMES
    ]
    ipcs = [overall_ipc(path) for path in benchmark_paths]

    charts.bar_chart(
        f"IPC for selected SPEC benchmarks",
        "Benchmark",
        "IPC",
        benchmark_names,
        ipcs,
        out_filepath,
    )


def main():
    for benchmark_name in BENCHMARK_NAMES:
        print(f"Running benchmark {benchmark_name}")
        run_benchmark(benchmark_name)

    print(
        "Generating chart for IPC of SPEC benchmarks... ",end="", flush=True
    )
    start = time.perf_counter()
    ipc_chart(BENCHMARK_NAMES, OUTPUT_DIRPATH / "ipcchart.png")
    end = time.perf_counter()
    print(f"Done in {(end - start)*1000:.3f} ms")


if __name__ == "__main__":
    main()
