#!/usr/bin/env python3
from pathlib import Path


def main() -> None:
    samples_dir = Path("samples")
    samples_dir.mkdir(exist_ok=True)

    repetitive = ("A" * 120 + "BBBBCCCCDDDD" + "\n") * 12000
    mixed_lines = []
    for i in range(8000):
        mixed_lines.append(
            f"Linea {i} | usuario=so{i%17} | proceso={1000+i} | estado=RUNNING | mensaje=kernel-user-space-io\n"
        )
    mixed = "".join(mixed_lines)

    pseudo_natural = (
        "En sistemas operativos, la compresion previa al write reduce trafico en el bus de I/O. "
        "Esta frase se repite con pequenas variaciones para simular texto real.\n"
    ) * 14000

    (samples_dir / "repetitive_2mb.txt").write_text(repetitive, encoding="utf-8")
    (samples_dir / "mixed_2mb.txt").write_text(mixed, encoding="utf-8")
    (samples_dir / "natural_2mb.txt").write_text(pseudo_natural, encoding="utf-8")

    print("Muestras listas en samples/")


if __name__ == "__main__":
    main()
