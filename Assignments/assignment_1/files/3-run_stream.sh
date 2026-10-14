#!/bin/bash
# run_stream.sh
# Compiles and runs the STREAM benchmark (copy/scale/add/triad) across a
# sweep of OpenMP thread counts, and writes a CSV summary.
#
# Usage:
#   ./run_stream.sh
#
# Requires: gcc (or icc) with OpenMP support, stream.c in the same directory.

set -e

THREADS="1 2 4 8 16"
ARRAY_SIZE=20000000     # elements per array; increase if runs finish too fast
OUTDIR="stream_results"
CSV="$OUTDIR/summary.csv"

mkdir -p "$OUTDIR"

echo "Compiling STREAM (array size = $ARRAY_SIZE elements per array)..."
gcc -O3 -fopenmp -DSTREAM_ARRAY_SIZE=$ARRAY_SIZE -o stream 3-stream.c -lm
# Intel compiler alternative:
#   icc -O3 -openmp -DSTREAM_ARRAY_SIZE=$ARRAY_SIZE -o stream stream.c

echo "threads,copy_mbps,scale_mbps,add_mbps,triad_mbps" > "$CSV"

for T in $THREADS; do
    echo "Running with OMP_NUM_THREADS=$T ..."
    OUT="$OUTDIR/stream_${T}threads.out"
    OMP_NUM_THREADS=$T ./stream > "$OUT"

    COPY=$(grep '^Copy:'  "$OUT" | awk '{print $2}')
    SCALE=$(grep '^Scale:' "$OUT" | awk '{print $2}')
    ADD=$(grep '^Add:'   "$OUT" | awk '{print $2}')
    TRIAD=$(grep '^Triad:' "$OUT" | awk '{print $2}')

    echo "$T,$COPY,$SCALE,$ADD,$TRIAD" >> "$CSV"
done

echo ""
echo "Done. Per-run output in $OUTDIR/, summary table in $CSV:"
echo ""
if command -v column >/dev/null 2>&1; then
    column -s, -t "$CSV"
else
    cat "$CSV"
fi
