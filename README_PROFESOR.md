# README PARA PROFESOR - Parcial 3

**Materia:** Sistemas Operativos  
**Parcial:** #3 - Seguridad y Comprimir  
**Autores:** Maximiliano Bustamante y Valeria Hornung

## 0. Entorno, dependencias e instalacion

Proyecto preparado para **Linux nativo** o **WSL (Ubuntu)**.

Dependencias minimas:

- compilador C (`gcc` o `cc`),
- `make`,
- `bash`,
- `python3`,
- `strace`,
- `/usr/bin/time` (paquete `time`),
- `zlib` de desarrollo (`zlib1g-dev`),
- opcional: `python3-matplotlib` para graficas.

Instalacion recomendada (Ubuntu/WSL):

```bash
sudo apt update
sudo apt install -y build-essential make bash python3 strace time zlib1g-dev python3-matplotlib
```

Verificacion rapida:

```bash
gcc --version
python3 --version
strace -V
/usr/bin/time --version
```

## 1. Compilacion y ejecucion

Compilar todo:

```bash
make clean
make
```

Binarios generados:

- `./editor` (editor interactivo),
- `./pipeline_cli` (pipeline no interactivo para pruebas),
- `./plain_writer` (baseline clasico con writes pequenos).

Ejecutar editor:

```bash
./editor
```

Comandos principales dentro del editor:

- `append <texto>`
- `insert <indice> <texto>`
- `delete <indice>`
- `print`
- `save <archivo.bin> <rle|huffman|lzw|deflate> <write|mmap>`
- `open <archivo.bin>`
- `stats`
- `quit`

## 2. Objetivo tecnico

Se desarrollo un editor en C para Linux con pipeline de optimizacion I/O:

- Edicion en memoria de texto.
- Compresion en user-space antes de invocar syscalls de escritura.
- Escritura a disco mediante:
  - `write` en bloques de 4KB,
  - `mmap` + `msync`.
- Formato binario propio con header empaquetado para separar metadatos y payload comprimido.

## 3. Matriz de diseno del pipeline I/O

Flujo implementado:

1. Entrada de texto en consola.
2. Estructura dinamica en memoria (`text_buffer_t`).
3. Serializacion a bloque plano en RAM.
4. Compresion (`RLE`, `Huffman`, `LZW` o `Deflate`).
5. Construccion de header binario empaquetado.
6. Persistencia:
   - opcion A: `write()` en bloques 4096 bytes,
   - opcion B: `mmap()` para escritura mapeada.
7. Carga:
   - lectura de header + payload,
   - descompresion en RAM,
   - reconstruccion del buffer editable.

## 4. Gestion de memoria en C

- Uso controlado de memoria dinamica en todos los modulos:
  - `malloc/realloc/free` en estructuras de texto,
  - buffers intermedios para compresion/descompresion,
  - liberacion explicita en rutas de error.
- Header con `__attribute__((packed))` para evitar padding no deseado y mantener formato deterministico.
- Recomendacion de validacion:

```bash
valgrind --leak-check=full ./editor
```

## 5. Funcionalidad del editor

Editor de consola con comandos:

- `append`, `insert`, `delete`, `print`, `stats`,
- `save <archivo.bin> <algoritmo> <modo_io>`,
- `open <archivo.bin>`.

La persistencia se realiza en binario comprimido, sin guardar texto claro al disco desde el flujo del editor.

## 6. Profiling, benchmark y evidencia de rendimiento

Se automatizo benchmark con:

- `strace -c` para conteo de syscalls,
- `/usr/bin/time` para tiempos `real`, `user`, `sys`.

Comando:

```bash
make benchmark
```

Artefactos generados:

- `reports/raw/*.strace.txt`
- `reports/raw/*.time.txt`
- `reports/summary.csv`
- `reports/PROFILING_REPORT.md`
- `reports/figures/*.png`
- `reports/GRAPH_SUMMARY.md`

Generar/actualizar graficas manualmente:

```bash
python3 scripts/plot_reports.py
```

Ver graficas desde WSL en Windows:

```bash
explorer.exe reports/figures
```

## 7. Comparativa de metodos incluidos

Metodos de clase:

- RLE
- Huffman
- LZW

Metodo adicional (mas profesional):

- Deflate (zlib)

Se comparan en dos backends de I/O (`write` y `mmap`) para observar trade-off CPU vs ahorro de bus I/O.

## 8. Guia rapida de reproduccion completa

Secuencia corta para evaluacion:

```bash
make clean
make
make benchmark
python3 scripts/plot_reports.py
```

Luego revisar:

- `reports/PROFILING_REPORT.md`
- `reports/summary.csv`
- `reports/GRAPH_SUMMARY.md`
- `reports/figures/*.png`

## 9. Distribucion del trabajo entre estudiantes

### Maximiliano Bustamante

- Implementacion del editor por consola y estructura de memoria (`text_buffer_t`).
- Integracion de syscalls POSIX de I/O (`open/read/write`, `mmap`, `msync`).
- Implementacion de baseline de referencia (`plain_writer`) para contraste experimental.

### Valeria Hornung

- Implementacion de modulos de compresion/descompresion (RLE, Huffman, LZW y Deflate).
- Diseno de formato de archivo binario con header empaquetado y validaciones.
- Automatizacion de benchmark y generacion de reportes (CSV/Markdown/graficas).

> Nota: ambos participaron en pruebas integrales y documentacion final.

## 10. Conclusiones finales (corrida ejecutada)

Con base en `reports/summary.csv`, `reports/PROFILING_REPORT.md` y `reports/GRAPH_SUMMARY.md`, se obtuvo:

- Baseline clasico (`baseline_small_writes`):
  - `real = 4.53s`,
  - `write() = 11035`,
  - ratio `1.000`.
- Mejor rendimiento total:
  - `pack_deflate_write` y `pack_deflate_mmap` con `real = 0.02s`.
- Mejor relacion de compresion:
  - `deflate` con ratio `0.084` (archivo final ~8.4% del original).

Impacto cuantitativo frente al baseline:

- Mejora de tiempo wall-clock: **99.56%**.
- Reduccion de llamadas `write()`: **99.86%**.
- Reduccion de volumen escrito a disco (caso deflate): cercana al **91.6%**.

Veredicto tecnico:

- Para este dataset, comprimir en user-space antes de escribir al kernel fue claramente rentable.
- El costo extra de CPU por compresion fue bajo frente al ahorro de I/O.
- La combinacion recomendada para entrega es **Deflate + I/O por bloques (`write`)** por simplicidad operativa y rendimiento sobresaliente; `Deflate + mmap` tambien obtuvo el mejor tiempo, por lo que ambos enfoques cumplen el objetivo de optimizacion.
