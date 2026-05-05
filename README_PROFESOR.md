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

## 10. Conclusiones (a actualizar con corrida final)

Tras ejecutar `make benchmark` en Linux, se espera:

- reduccion importante de `write()` calls frente a baseline clasico,
- reduccion del volumen en disco en los algoritmos con mejor ratio,
- aumento moderado de `user time` por compresion,
- mejora neta de `real time` en combinaciones que reduzcan suficiente I/O.

La conclusion definitiva se apoya en los valores exactos de `reports/summary.csv` y `reports/PROFILING_REPORT.md`.
