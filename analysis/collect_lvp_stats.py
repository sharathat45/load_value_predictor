import shutil
import subprocess
import pathlib
import time
from typing import List, Dict, Union
import sys

import pandas as pd

from m5stats import M5Stats

import charts

M5_DIRPATH = pathlib.Path("../m5out")

STATS_DIRPATH = pathlib.Path("stats")
OUTPUT_DIRPATH = pathlib.Path("charts")

LVPSTATS_PATH = "system.cpu.lvp"

BENCHMARK_NAMES = ("sjeng", "leslie3d", "lbm", "astar", "milc", "namd")

STATSFILE_SUFFIX = "_stats.txt"

C_LOOKUP = 'lookups'
C_PRED_TOTAL = 'predTotal'
C_PRED_CORRECT = 'predCorrect'
C_PRED_INCORRECT = 'predIncorrect'
C_PRED_RATE = 'predRate'
C_PRED_ACC = 'predAccuracy'
C_CONST_PRED = 'constPred'
C_CONST_INV = 'constInval'
C_CONST_RB = 'constRollback'
C_CONST_RBRATE = 'constRollbackRate'


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


def read_lvp_stats(benchmark_names: List[str]) -> pd.DataFrame:
    lvp_stats = {}

    for benchmark_name in benchmark_names:
        benchmark_path = STATS_DIRPATH / (benchmark_name + STATSFILE_SUFFIX)
        lvp_stats[benchmark_name] = M5Stats(benchmark_path).get_stats(LVPSTATS_PATH).as_dict()
    
    return pd.DataFrame.from_dict(lvp_stats, orient='index')


def pred_rate_stats_table(lvp_stats: pd.DataFrame, out_filepath: str = None) -> pd.DataFrame:
    pred_rate_stats = pd.DataFrame()

    pred_rate_stats['Lookups'] = lvp_stats[C_LOOKUP]
    pred_rate_stats['Predictions'] = lvp_stats[C_PRED_TOTAL]

    pred_rate_stats['Prediction rate'] = lvp_stats[C_PRED_TOTAL]/lvp_stats[C_LOOKUP]
    pred_rate_stats['Prediction rate'] = pred_rate_stats['Prediction rate'].map(lambda x: f"{100*x:.2f}\\%")

    if out_filepath is not None:
        with open(out_filepath, 'w') as f:
            f.write(str(pred_rate_stats))
            f.write("\n\n" + "*" * 80 + "\n\n")
            pred_rate_stats.to_latex(f, caption="LVP Prediction Rate")
    
    return pred_rate_stats


def pred_acc_stats_table(lvp_stats: pd.DataFrame, out_filepath: str = None) -> pd.DataFrame:
    pred_acc_stats = pd.DataFrame()

    pred_acc_stats['Dynamic pred'] = lvp_stats[C_PRED_TOTAL]
    pred_acc_stats['Committed pred'] = lvp_stats[C_PRED_CORRECT] + lvp_stats[C_PRED_INCORRECT]

    pred_acc_stats['Correct'] = lvp_stats[C_PRED_CORRECT]
    pred_acc_stats['Incorrect'] = lvp_stats[C_PRED_INCORRECT]

    pred_acc_stats['Accuracy'] = lvp_stats[C_PRED_CORRECT]/pred_acc_stats['Committed pred']
    pred_acc_stats['Accuracy'] = pred_acc_stats['Accuracy'].map(lambda x: f"{100*x:.2f}\\%")

    if out_filepath is not None:
        with open(out_filepath, 'w') as f:
            f.write(str(pred_acc_stats))
            f.write("\n\n" + "*" * 80 + "\n\n")
            pred_acc_stats.to_latex(f, caption="LVP Prediction Accuracy")
    
    return pred_acc_stats


def pred_const_stats_table(lvp_stats: pd.DataFrame, out_filepath: str) -> pd.DataFrame:
    pred_const_stats = pd.DataFrame()

    pred_const_stats['Dynamic predictions'] = lvp_stats[C_PRED_TOTAL]
    pred_const_stats['Constant predictions'] = lvp_stats[C_CONST_PRED]

    pred_const_stats['Constant fraction'] = lvp_stats[C_CONST_PRED]/lvp_stats[C_PRED_TOTAL]
    pred_const_stats['Constant fraction'] = pred_const_stats['Constant fraction'].map(lambda x: f"{100*x:.2f}\\%")

    if out_filepath is not None:
        with open(out_filepath, 'w') as f:
            f.write(str(pred_const_stats))
            f.write("\n\n" + "*" * 80 + "\n\n")
            pred_const_stats.to_latex(f, caption="LVP Constant Predictions")
    
    return pred_const_stats


def mem_access_red_table(lvp_stats: pd.DataFrame, out_filepath: str) -> pd.DataFrame:
    mem_access_red_stats = pd.DataFrame()

    mem_access_red_stats['Loads from L1$ without LVP'] = lvp_stats[C_LOOKUP]
    mem_access_red_stats['Loads from L1$ with LVP'] = lvp_stats[C_LOOKUP] - lvp_stats[C_CONST_PRED]

    mem_access_red_stats['L1$ loads prevented'] = (
        mem_access_red_stats['Loads from L1$ without LVP'] - mem_access_red_stats['Loads from L1$ with LVP']
    )/mem_access_red_stats['Loads from L1$ without LVP']
    mem_access_red_stats['L1$ loads prevented'] = mem_access_red_stats['L1$ loads prevented'].map(lambda x: f"{100*x:.2f}\\%")

    if out_filepath is not None:
        with open(out_filepath, 'w') as f:
            f.write(str(mem_access_red_stats))
            f.write("\n\n" + "*" * 80 + "\n\n")
            mem_access_red_stats.to_latex(f, caption="LVP Constant Predictions")
    
    return mem_access_red_stats


def main():
    program_start = time.perf_counter()

    for benchmark_name in BENCHMARK_NAMES:
        start = time.perf_counter()

        if len(sys.argv) > 1 and sys.argv[1] == "rerun":
            print(f"Running benchmark {benchmark_name}")
            run_benchmark(benchmark_name)

        print(f"Generating chart for benchmark {benchmark_name}")
        output_path = OUTPUT_DIRPATH / f"{benchmark_name}_lvp.png"
        lvp_chart(benchmark_name, output_path)

        end = time.perf_counter()
        print(f"Done in {(end - start)*1000:.3f} ms")

    print("Parsing LVP stats")
    lvp_stats = read_lvp_stats(BENCHMARK_NAMES)

    print("Generating prediction rate table")
    pred_rate_stats_table(lvp_stats, OUTPUT_DIRPATH / "lvp_pred_rate.txt")

    print("Generating prediction accuracy table")
    pred_acc_stats_table(lvp_stats, OUTPUT_DIRPATH / "lvp_pred_acc.txt")

    print("Generating constant predictions table")
    pred_const_stats_table(lvp_stats, OUTPUT_DIRPATH / "lvp_pred_const.txt")

    print("Generating mem access table")
    mem_access_red_table(lvp_stats, OUTPUT_DIRPATH / "lvp_mem_access.txt")

    program_end = time.perf_counter()
    print(f"Finished in {(program_end - program_start)*1000:.3f} ms")


if __name__ == "__main__":
    main()
