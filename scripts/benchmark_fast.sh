#!/usr/bin/env bash
set -euo pipefail

mkdir -p reports/raw out samples

if ! command -v strace >/dev/null 2>&1; then
  echo "Error: strace no esta instalado."
  exit 1
fi

if ! command -v /usr/bin/time >/dev/null 2>&1; then
  echo "Error: /usr/bin/time no esta disponible."
  exit 1
fi

python3 scripts/generate_samples.py

RUNS_FILE="reports/raw/runs.csv"
echo "tag,kind,input_path,output_path,algo,io,input_bytes,output_bytes,strace_file,time_file" > "$RUNS_FILE"

run_case() {
  local tag="$1"
  local kind="$2"
  local input_path="$3"
  local output_path="$4"
  local algo="$5"
  local io="$6"
  shift 6
  local strace_file="reports/raw/${tag}.strace.txt"
  local time_file="reports/raw/${tag}.time.txt"

  rm -f "$output_path"
  /usr/bin/time -f "real=%e\nuser=%U\nsys=%S" -o "$time_file" \
    strace -c -o "$strace_file" "$@"

  local input_bytes
  local output_bytes
  input_bytes=$(stat -c%s "$input_path")
  output_bytes=$(stat -c%s "$output_path")

  echo "${tag},${kind},${input_path},${output_path},${algo},${io},${input_bytes},${output_bytes},${strace_file},${time_file}" >> "$RUNS_FILE"
  echo "OK ${tag}"
}

INPUT="samples/mixed_2mb.txt"

run_case "baseline_small_writes" "baseline" "$INPUT" "out/plain_output.txt" "none" "small_write" \
  ./plain_writer "$INPUT" "out/plain_output.txt"

for algo in rle huffman lzw deflate; do
  for io in write mmap; do
    tag="pack_${algo}_${io}"
    output="out/${tag}.bin"
    run_case "$tag" "compressed" "$INPUT" "$output" "$algo" "$io" \
      ./pipeline_cli pack "$INPUT" "$output" "$algo" "$io"
  done
done

# Verificacion de integridad (rapida): 4 pruebas de unpack.
for algo in rle huffman lzw deflate; do
  input_bin="out/pack_${algo}_write.bin"
  output_txt="out/recovered_${algo}.txt"
  ./pipeline_cli unpack "$input_bin" "$output_txt"
  cmp -s "$INPUT" "$output_txt"
  echo "Integridad OK para ${algo}"
done

python3 scripts/generate_report.py
echo "Benchmark finalizado. Revisar reports/PROFILING_REPORT.md"
