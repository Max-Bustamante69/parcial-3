#!/usr/bin/env python3
import csv
from pathlib import Path


def parse_time_file(path: Path) -> dict:
    data = {"real": 0.0, "user": 0.0, "sys": 0.0}
    if not path.exists():
        return data
    for line in path.read_text(encoding="utf-8").splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        try:
            data[key.strip()] = float(value.strip())
        except ValueError:
            pass
    return data


def parse_strace_file(path: Path) -> dict:
    write_calls = 0
    total_calls = 0
    if not path.exists():
        return {"write_calls": write_calls, "total_calls": total_calls}

    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("% time") or stripped.startswith("------"):
            continue
        parts = stripped.split()
        if not parts:
            continue

        syscall = parts[-1]
        calls = 0
        if syscall == "total":
            for token in parts:
                if token.isdigit():
                    calls = int(token)
                    break
            total_calls = calls
            continue

        if len(parts) >= 5 and parts[3].isdigit():
            calls = int(parts[3])
        elif len(parts) >= 4 and parts[2].isdigit():
            calls = int(parts[2])
        if syscall == "write":
            write_calls = calls

    return {"write_calls": write_calls, "total_calls": total_calls}


def main() -> None:
    raw_runs = Path("reports/raw/runs.csv")
    reports_dir = Path("reports")
    reports_dir.mkdir(exist_ok=True)

    if not raw_runs.exists():
        raise SystemExit("No existe reports/raw/runs.csv. Ejecuta benchmark primero.")

    rows = []
    with raw_runs.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            time_data = parse_time_file(Path(row["time_file"]))
            strace_data = parse_strace_file(Path(row["strace_file"]))
            input_bytes = int(row["input_bytes"])
            output_bytes = int(row["output_bytes"])
            ratio = (output_bytes / input_bytes) if input_bytes else 0.0
            rows.append(
                {
                    **row,
                    "ratio": ratio,
                    "real_s": time_data["real"],
                    "user_s": time_data["user"],
                    "sys_s": time_data["sys"],
                    "write_calls": strace_data["write_calls"],
                    "total_syscalls": strace_data["total_calls"],
                }
            )

    summary_csv = reports_dir / "summary.csv"
    with summary_csv.open("w", encoding="utf-8", newline="") as f:
        fields = [
            "tag",
            "kind",
            "algo",
            "io",
            "input_bytes",
            "output_bytes",
            "ratio",
            "write_calls",
            "total_syscalls",
            "real_s",
            "user_s",
            "sys_s",
        ]
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        for row in rows:
            writer.writerow({k: row[k] for k in fields})

    baseline = next((r for r in rows if r["kind"] == "baseline"), None)
    compressed = [r for r in rows if r["kind"] == "compressed"]
    best = min(compressed, key=lambda r: (r["real_s"], r["ratio"])) if compressed else None

    md = []
    md.append("# Reporte de Profiling")
    md.append("")
    md.append("## Metodologia")
    md.append("- Medicion automatica con `strace -c` y `/usr/bin/time`.")
    md.append("- Archivo evaluado: `samples/mixed_2mb.txt`.")
    md.append("- Comparacion entre baseline (writes pequenos sin compresion) y pipeline comprimido.")
    md.append("")
    md.append("## Resultados clave")
    if baseline:
        md.append(
            f"- Baseline: `write_calls={baseline['write_calls']}`, `total_syscalls={baseline['total_syscalls']}`, "
            f"`real={baseline['real_s']:.4f}s`."
        )
    if best:
        md.append(
            f"- Mejor corrida por tiempo total: `{best['tag']}` con `real={best['real_s']:.4f}s`, "
            f"`ratio={best['ratio']:.3f}`, `write_calls={best['write_calls']}`."
        )
    md.append("")
    md.append("## Tabla resumida")
    md.append("")
    md.append("| Tag | Algoritmo | I/O | Ratio | write() calls | Syscalls totales | Real(s) | User(s) | Sys(s) |")
    md.append("|---|---|---|---:|---:|---:|---:|---:|---:|")
    for r in rows:
        md.append(
            f"| {r['tag']} | {r['algo']} | {r['io']} | {r['ratio']:.3f} | "
            f"{r['write_calls']} | {r['total_syscalls']} | {r['real_s']:.4f} | {r['user_s']:.4f} | {r['sys_s']:.4f} |"
        )

    md.append("")
    md.append("## Veredicto")
    md.append(
        "- Se recomienda elegir la combinacion que minimice `real_s` manteniendo una `ratio` baja y reduciendo llamadas a `write()`."
    )
    md.append("- Ver `reports/summary.csv` para analisis adicional o graficas personalizadas.")

    (reports_dir / "PROFILING_REPORT.md").write_text("\n".join(md), encoding="utf-8")

    try:
        import matplotlib.pyplot as plt  # type: ignore
    except Exception:
        print("Reporte generado sin graficas (matplotlib no instalado).")
        return

    labels = [r["tag"] for r in rows]
    real_values = [r["real_s"] for r in rows]
    ratio_values = [r["ratio"] for r in rows]
    write_values = [r["write_calls"] for r in rows]

    plt.figure(figsize=(11, 4))
    plt.bar(labels, real_values)
    plt.xticks(rotation=45, ha="right")
    plt.ylabel("Segundos (real)")
    plt.title("Tiempo wall-clock por corrida")
    plt.tight_layout()
    plt.savefig(reports_dir / "real_time.png", dpi=120)
    plt.close()

    plt.figure(figsize=(11, 4))
    plt.bar(labels, ratio_values)
    plt.xticks(rotation=45, ha="right")
    plt.ylabel("output/input")
    plt.title("Ratio de compresion por corrida")
    plt.tight_layout()
    plt.savefig(reports_dir / "compression_ratio.png", dpi=120)
    plt.close()

    plt.figure(figsize=(11, 4))
    plt.bar(labels, write_values)
    plt.xticks(rotation=45, ha="right")
    plt.ylabel("Cantidad de write()")
    plt.title("Llamadas write() por corrida")
    plt.tight_layout()
    plt.savefig(reports_dir / "write_calls.png", dpi=120)
    plt.close()

    print("Reporte y graficas generadas en reports/")


if __name__ == "__main__":
    main()
