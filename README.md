# Parcial 3 - Editor con Pipeline de Compresion

Autores del trabajo: **Maximiliano Bustamante** y **Valeria Hornung**.

Este README es para ustedes dos: explica rapido que hace cada parte del codigo y como correr todo por consola de forma simple.

## 1) Que resuelve este proyecto

Se implemento un editor de texto por consola en C que:

- edita texto en memoria (append/insert/delete/print),
- guarda archivos en formato binario propio,
- **nunca escribe texto plano al disco** al guardar desde el editor,
- comprime en user-space antes de hacer syscalls de I/O,
- permite comparar dos estrategias de I/O: `write` en bloques de 4KB y `mmap`.

## 2) Algoritmos de compresion incluidos

Se implementaron 4 metodos:

- `rle` (visto en clase),
- `huffman` (visto en clase),
- `lzw` (visto en clase),
- `deflate` (metodo mas profesional, via zlib).

> Nota: `deflate` requiere `zlib` instalada en Linux (`-lz`).

## 3) Estructura del proyecto

- `src/main.c`: editor interactivo por consola.
- `src/editor.c`: estructura en memoria (lineas dinamicas) + abrir/guardar formato binario.
- `src/io_backend.c`: escritura por `write` (4KB) y `mmap`.
- `src/compression_*.c`: implementaciones de compresion/descompresion.
- `src/pipeline_cli.c`: utilidad no interactiva para benchmarks (`pack`/`unpack`).
- `src/plain_writer.c`: baseline clasico con `write` pequenos (simula enfoque ineficiente).
- `scripts/benchmark_fast.sh`: corre benchmark rapido con `strace -c` + `time`.
- `scripts/generate_report.py`: genera resumen CSV + reporte Markdown (+ graficas opcionales).
- `include/file_format.h`: header binario empaquetado (`__attribute__((packed))`).

## 4) Comandos principales (Linux)

### Compilar

```bash
make clean
make
```

### Ejecutar editor interactivo

```bash
./editor
```

Comandos dentro del editor:

- `append <texto>`
- `insert <indice> <texto>`
- `delete <indice>`
- `print`
- `open <archivo.bin>`
- `save <archivo.bin> <rle|huffman|lzw|deflate> <write|mmap>`
- `stats`
- `quit`

Ejemplo corto:

```text
append Hola Maxi y Vale
append Este texto se comprime antes de guardar
save out/notas.bin huffman write
clear
open out/notas.bin
print
```

### Utilidad directa para pruebas de pipeline

```bash
./pipeline_cli pack samples/mixed_2mb.txt out/test.bin deflate mmap
./pipeline_cli unpack out/test.bin out/recovered.txt
```

## 5) Benchmark rapido (menos de 1 minuto)

```bash
make benchmark
```

Este comando:

1. genera muestras de texto pequenas/medias,
2. ejecuta baseline clasico,
3. prueba 4 algoritmos x 2 modos de I/O,
4. valida integridad descomprimiendo,
5. genera reportes.

Archivos de salida:

- `reports/PROFILING_REPORT.md`
- `reports/summary.csv`
- `reports/raw/*.strace.txt`
- `reports/raw/*.time.txt`
- `reports/*.png` (si hay `matplotlib`)

## 6) Formato binario del archivo

Cada archivo guardado usa:

1. **Header empaquetado** (`editor_file_header_t`):
   - magic,
   - version,
   - algoritmo,
   - modo I/O usado,
   - tamano original,
   - tamano comprimido.
2. **Payload comprimido**.

Esto separa metadatos del contenido y evita guardar texto claro directamente.

## 7) Memoria y seguridad

- Uso de `malloc/realloc/free` controlado en cada modulo.
- Buffers intermedios para compresion/descompresion.
- Validaciones de tamano y formato al abrir archivos.
- Recomendado para validar fugas:

```bash
valgrind --leak-check=full ./editor
```

## 8) Conclusiones finales (con reportes reales)

Resultados obtenidos con los reportes finales en `reports/`:

- **Baseline clasico** (`plain_writer`): `real=4.53s`, `write_calls=11035`, ratio `1.0`.
- **Mejor pipeline por tiempo**: `pack_deflate_write` con `real=0.02s`, `ratio=0.084`, `write_calls=16`.
- **Mejora de tiempo total**: `99.56%` frente al baseline.
- **Reduccion de llamadas a write()**: `99.86%` frente al baseline.
- **Mejor compresion**: `deflate` (`0.084`), seguido de `lzw` (`0.162`), `huffman` (`0.596`), y `rle` (en este dataset no conviene, `1.963`).

Interpretacion tecnica:

- En este caso de prueba, invertir CPU en compresion (user space) redujo fuertemente el trafico de I/O.
- Menos bytes + menos llamadas `write()` = menor costo total en tiempo wall-clock.
- `deflate` fue el metodo mas rentable globalmente para este dataset.

Evidencia exacta en:

- `reports/summary.csv`
- `reports/PROFILING_REPORT.md`
- `reports/GRAPH_SUMMARY.md`
- `reports/figures/*.png`
