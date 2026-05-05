#!/usr/bin/env python3
import csv
from pathlib import Path
from statistics import mean

import matplotlib.pyplot as plt


def load_rows(path: Path) -> list[dict]:
    with path.open("r", encoding="utf-8", newline="") as f:
        return list(csv.DictReader(f))


def to_float(row: dict, key: str) -> float:
    return float(row.get(key, 0.0) or 0.0)


def to_int(row: dict, key: str) -> int:
    return int(float(row.get(key, 0) or 0))


def make_bar(labels: list[str], values: list[float], title: str, ylabel: str, out: Path) -> None:
    plt.figure(figsize=(12, 4.5))
    bars = plt.bar(labels, values)
    plt.xticks(rotation=35, ha="right")
    plt.title(title)
    plt.ylabel(ylabel)
    for bar, v in zip(bars, values):
        plt.text(bar.get_x() + bar.get_width() / 2, bar.get_height(), f"{v:.3f}", ha="center", va="bottom", fontsize=8)
    plt.tight_layout()
    plt.savefig(out, dpi=140)
    plt.close()


def main() -> None:
    reports_dir = Path("reports")
    figures_dir = reports_dir / "figures"
    figures_dir.mkdir(parents=True, exist_ok=True)

    summary_path = reports_dir / "summary.csv"
    if not summary_path.exists():
        raise SystemExit("No existe reports/summary.csv. Ejecuta primero: make benchmark")

    rows = load_rows(summary_path)
    baseline = next((r for r in rows if r["kind"] == "baseline"), None)
    compressed = [r for r in rows if r["kind"] == "compressed"]
    if not baseline or not compressed:
        raise SystemExit("summary.csv no tiene baseline/compressed suficiente.")

    all_labels = [r["tag"] for r in rows]
    real_values = [to_float(r, "real_s") for r in rows]
    ratio_values = [to_float(r, "ratio") for r in rows]
    write_values = [to_int(r, "write_calls") for r in rows]
    syscall_values = [to_int(r, "total_syscalls") for r in rows]

    make_bar(
        all_labels,
        real_values,
        "Tiempo total (real) por corrida",
        "segundos",
        figures_dir / "01_real_time_all.png",
    )
    make_bar(
        all_labels,
        ratio_values,
        "Ratio de compresion (output/input)",
        "ratio",
        figures_dir / "02_compression_ratio_all.png",
    )
    make_bar(
        all_labels,
        [float(v) for v in write_values],
        "Cantidad de llamadas write()",
        "llamadas",
        figures_dir / "03_write_calls_all.png",
    )
    make_bar(
        all_labels,
        [float(v) for v in syscall_values],
        "Total syscalls reportadas por strace -c",
        "llamadas",
        figures_dir / "04_total_syscalls_all.png",
    )

    by_algo = {}
    for r in compressed:
        key = r["algo"]
        by_algo.setdefault(key, []).append(r)

    algo_labels = []
    algo_best_time = []
    algo_best_ratio = []
    for algo, vals in sorted(by_algo.items()):
        best_time = min(vals, key=lambda x: to_float(x, "real_s"))
        best_ratio = min(vals, key=lambda x: to_float(x, "ratio"))
        algo_labels.append(algo)
        algo_best_time.append(to_float(best_time, "real_s"))
        algo_best_ratio.append(to_float(best_ratio, "ratio"))

    plt.figure(figsize=(10, 4.5))
    x = range(len(algo_labels))
    w = 0.38
    plt.bar([i - w / 2 for i in x], algo_best_time, width=w, label="Mejor tiempo real (s)")
    plt.bar([i + w / 2 for i in x], algo_best_ratio, width=w, label="Mejor ratio compresion")
    plt.xticks(list(x), algo_labels)
    plt.title("Comparacion por algoritmo (mejor corrida por metrica)")
    plt.legend()
    plt.tight_layout()
    plt.savefig(figures_dir / "05_algorithm_comparison.png", dpi=140)
    plt.close()

    baseline_real = to_float(baseline, "real_s")
    best_pipeline = min(compressed, key=lambda r: (to_float(r, "real_s"), to_float(r, "ratio")))
    best_real = to_float(best_pipeline, "real_s")
    speedup_pct = (baseline_real - best_real) / baseline_real * 100 if baseline_real > 0 else 0.0

    baseline_write = to_int(baseline, "write_calls")
    best_write = to_int(best_pipeline, "write_calls")
    write_reduction_pct = (baseline_write - best_write) / baseline_write * 100 if baseline_write > 0 else 0.0

    quick_path = reports_dir / "GRAPH_SUMMARY.md"
    quick = []
    quick.append("# Resumen de graficas")
    quick.append("")
    quick.append(f"- Baseline: `{baseline['tag']}`")
    quick.append(f"- Mejor pipeline por tiempo: `{best_pipeline['tag']}`")
    quick.append(f"- Mejora de tiempo real: `{speedup_pct:.2f}%`")
    quick.append(f"- Reduccion write(): `{write_reduction_pct:.2f}%`")
    quick.append(f"- Ratio promedio (corridas comprimidas): `{mean(to_float(r, 'ratio') for r in compressed):.3f}`")
    quick.append("")
    quick.append("## Graficas generadas")
    quick.append("- `reports/figures/01_real_time_all.png`")
    quick.append("- `reports/figures/02_compression_ratio_all.png`")
    quick.append("- `reports/figures/03_write_calls_all.png`")
    quick.append("- `reports/figures/04_total_syscalls_all.png`")
    quick.append("- `reports/figures/05_algorithm_comparison.png`")
    quick_path.write_text("\n".join(quick), encoding="utf-8")

    print("Graficas generadas en reports/figures")
    print("Resumen rapido en reports/GRAPH_SUMMARY.md")


if __name__ == "__main__":
    main()
