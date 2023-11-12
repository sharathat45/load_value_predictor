from typing import List
import pathlib
import matplotlib.pyplot as plt

from m5stats import M5Stats

INSTR_MIX_PATH = "system.cpu.exec_context.thread_0.statExecutedInstType"


def bar_chart(
    title: str,
    xlabel: str,
    ylabel: str,
    labels: List[str],
    values: List[str],
    out_filepath: pathlib.Path,
    label_percent: bool = False,
):
    fig, ax = plt.subplots()
    ax.bar(labels, values)
    ax.set_title(title)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)

    labels = ax.get_xticklabels()
    plt.setp(labels, rotation=45, horizontalalignment="right")

    total = sum(values)
    for i, v in enumerate(values):
        disp = f"{v/total*100:.1f}%" if label_percent else f"{v:.2f}"
        ax.text(i, v, disp, rotation=45)

    ystart, yend = ax.get_ylim()
    ax.set_ylim(0, float(f"{yend*1.2:.1g}"))

    fig.tight_layout()
    fig.savefig(out_filepath)


def instr_mix_chart(name, stats: M5Stats, out_filepath: pathlib.Path):
    instr_mix_stats = stats.get_stats(INSTR_MIX_PATH).as_dict()
    instr_mix_stats.pop("total")

    instr_mix_stats = {k: v for k, v in instr_mix_stats.items() if v}

    bar_chart(
        f"Instruction mix for {name}",
        "Instruction type",
        "Instruction count",
        list(instr_mix_stats.keys()),
        list(instr_mix_stats.values()),
        out_filepath,
        True,
    )
