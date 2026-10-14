#!/bin/bash
# Performance studies for the assignment. Adapts to the number of physical cores.
#
#   ./run_studies.sh                    # QUICK mode (T=10, 1 run each) - start here
#   FULL=1 ./run_studies.sh             # final numbers (T=100, best of 3) - slow, run once at the end
#   MAXP=4 ./run_studies.sh             # force the max process count
#   MPIFLAGS="--use-hwthread-cpus" ./run_studies.sh   # count hardware threads as slots
#   REPS=1 N0=256 T=10 ./run_studies.sh # quick smoke test
#
# Output: results.csv  (study,N,P,px,py,threads,no_comm,gflops)   errors.log (failed runs)

N0=${N0:-1024}
# Gflops is a rate, so a shorter simulated time T gives the same Gflops, only faster.
#   default (quick):  T=10,  1 run per configuration   -> for checking everything works / drafting
#   FULL=1:           T=100, best of 3 runs            -> the settings the assignment asks for
if [ "${FULL:-0}" = 1 ]; then T=${T:-100}; REPS=${REPS:-3}; else T=${T:-10}; REPS=${REPS:-1}; fi
PHYS=$(lscpu -p=CORE,SOCKET 2>/dev/null | grep -v '^#' | sort -u | wc -l)
[ "${PHYS:-0}" -ge 1 ] || PHYS=$(nproc)
MAXP=${MAXP:-$PHYS}
MPIFLAGS=${MPIFLAGS:-}
OUT=results.csv
: > errors.log
echo "Using up to $MAXP processes/cores (physical cores detected: $PHYS), N0=$N0, T=$T, REPS=$REPS"

# best Gflops over REPS runs of: mpirun -n P ./cardiacsim <args>   (empty string if all runs failed)
best() {
  local P=$1; shift
  local b="" g out
  for r in $(seq "$REPS"); do
    if ! out=$(mpirun $MPIFLAGS -n "$P" ./cardiacsim "$@" 2>>errors.log); then
      echo "FAILED: mpirun -n $P ./cardiacsim $*" >> errors.log
      continue
    fi
    g=$(echo "$out" | grep "Sustained Gflops" | awk '{print $NF}')
    [ -z "$g" ] && continue
    if [ -z "$b" ] || awk -v a="$g" -v c="$b" 'BEGIN{exit !(a>c)}'; then b=$g; fi
  done
  echo "$b"
}

echo "study,N,P,px,py,threads,no_comm,gflops" > "$OUT"

# serial baseline (best of REPS)
sb=""
for r in $(seq "$REPS"); do
  g=$(./serial -n "$N0" -t "$T" | grep "Sustained Gflops" | awk '{print $NF}')
  if [ -z "$sb" ] || awk -v a="$g" -v c="$sb" 'BEGIN{exit !(a>c)}'; then sb=$g; fi
done
echo "serial,$N0,1,1,1,1,0,$sb" >> "$OUT"

# process counts: 1, 2, 4, ... up to MAXP (and MAXP itself if not a power of two)
PLIST=""
p=1
while [ "$p" -le "$MAXP" ]; do PLIST="$PLIST $p"; p=$((p * 2)); done
case " $PLIST " in *" $MAXP "*) ;; *) PLIST="$PLIST $MAXP" ;; esac

# (a) strong scaling: every geometry px x py with px*py = P, no OpenMP
bestg=0; bpx=1; bpy=$MAXP
for P in $PLIST; do
  for py in $(seq 1 "$P"); do
    [ $((P % py)) -eq 0 ] || continue
    px=$((P / py))
    g=$(best "$P" -n "$N0" -t "$T" -x "$px" -y "$py")
    echo "strong,$N0,$P,$px,$py,1,0,$g" >> "$OUT"
    echo "  P=$P ${px}x${py}: ${g:-FAILED}"
    if [ "$P" -eq "$MAXP" ] && [ -n "$g" ] && awk -v a="$g" -v c="$bestg" 'BEGIN{exit !(a>c)}'; then
      bestg=$g; bpx=$px; bpy=$py
    fi
  done
done
echo "Best geometry at P=$MAXP: ${bpx}x${bpy} ($bestg Gflops)"

# (b) communication overhead on the best geometry; shrink N by sqrt(2) until overhead >= 25%
N=$N0
while [ "$N" -ge 32 ]; do
  w=$(best "$MAXP" -n "$N" -t "$T" -x "$bpx" -y "$bpy")
  wo=$(best "$MAXP" -n "$N" -t "$T" -x "$bpx" -y "$bpy" -k)
  echo "comm,$N,$MAXP,$bpx,$bpy,1,0,$w" >> "$OUT"
  echo "comm,$N,$MAXP,$bpx,$bpy,1,1,$wo" >> "$OUT"
  [ -n "$w" ] && [ -n "$wo" ] || break
  ov=$(awk -v a="$w" -v b="$wo" 'BEGIN{printf "%.3f", 1 - a/b}')
  echo "  N=$N overhead=$ov"
  awk -v o="$ov" 'BEGIN{exit !(o >= 0.25)}' && break
  N=$(awk -v n="$N" 'BEGIN{printf "%d", n/1.4142}')
done

# (c) MPI + OpenMP with 1D decomposition: processes x threads = MAXP
th=1
while [ "$th" -le "$MAXP" ]; do
  if [ $((MAXP % th)) -eq 0 ]; then
    P=$((MAXP / th))
    g=$(MPIFLAGS="$MPIFLAGS --bind-to none" best "$P" -n "$N0" -t "$T" -y "$P" -o "$th")
    echo "hybrid,$N0,$P,1,$P,$th,0,$g" >> "$OUT"
    echo "  hybrid $P procs x $th threads: ${g:-FAILED}"
  fi
  th=$((th * 2))
done

echo "done in ${SECONDS}s -> $OUT   (failed runs, if any, are listed in errors.log)"

# #!/bin/bash
# # Performance studies for the assignment. Adapts to the number of physical cores.
# #
# #   ./run_studies.sh                    # normal use
# #   MAXP=4 ./run_studies.sh             # force the max process count
# #   MPIFLAGS="--use-hwthread-cpus" ./run_studies.sh   # count hardware threads as slots
# #   REPS=1 N0=256 T=10 ./run_studies.sh # quick smoke test
# #
# # Output: results.csv  (study,N,P,px,py,threads,no_comm,gflops)   errors.log (failed runs)

# N0=${N0:-1024}
# T=${T:-100}
# REPS=${REPS:-3}                        # each configuration is run REPS times, best Gflops kept
# PHYS=$(lscpu -p=CORE,SOCKET 2>/dev/null | grep -v '^#' | sort -u | wc -l)
# [ "${PHYS:-0}" -ge 1 ] || PHYS=$(nproc)
# MAXP=${MAXP:-$PHYS}
# MPIFLAGS=${MPIFLAGS:-}
# OUT=results.csv
# : > errors.log
# echo "Using up to $MAXP processes/cores (physical cores detected: $PHYS), N0=$N0, T=$T, REPS=$REPS"

# # best Gflops over REPS runs of: mpirun -n P ./cardiacsim <args>   (empty string if all runs failed)
# best() {
#   local P=$1; shift
#   local b="" g out
#   for r in $(seq "$REPS"); do
#     if ! out=$(mpirun $MPIFLAGS -n "$P" ./cardiacsim "$@" 2>>errors.log); then
#       echo "FAILED: mpirun -n $P ./cardiacsim $*" >> errors.log
#       continue
#     fi
#     g=$(echo "$out" | grep "Sustained Gflops" | awk '{print $NF}')
#     [ -z "$g" ] && continue
#     if [ -z "$b" ] || awk -v a="$g" -v c="$b" 'BEGIN{exit !(a>c)}'; then b=$g; fi
#   done
#   echo "$b"
# }

# echo "study,N,P,px,py,threads,no_comm,gflops" > "$OUT"

# # serial baseline (best of REPS)
# sb=""
# for r in $(seq "$REPS"); do
#   g=$(./serial -n "$N0" -t "$T" | grep "Sustained Gflops" | awk '{print $NF}')
#   if [ -z "$sb" ] || awk -v a="$g" -v c="$sb" 'BEGIN{exit !(a>c)}'; then sb=$g; fi
# done
# echo "serial,$N0,1,1,1,1,0,$sb" >> "$OUT"

# # process counts: 1, 2, 4, ... up to MAXP (and MAXP itself if not a power of two)
# PLIST=""
# p=1
# while [ "$p" -le "$MAXP" ]; do PLIST="$PLIST $p"; p=$((p * 2)); done
# case " $PLIST " in *" $MAXP "*) ;; *) PLIST="$PLIST $MAXP" ;; esac

# # (a) strong scaling: every geometry px x py with px*py = P, no OpenMP
# bestg=0; bpx=1; bpy=$MAXP
# for P in $PLIST; do
#   for py in $(seq 1 "$P"); do
#     [ $((P % py)) -eq 0 ] || continue
#     px=$((P / py))
#     g=$(best "$P" -n "$N0" -t "$T" -x "$px" -y "$py")
#     echo "strong,$N0,$P,$px,$py,1,0,$g" >> "$OUT"
#     echo "  P=$P ${px}x${py}: ${g:-FAILED}"
#     if [ "$P" -eq "$MAXP" ] && [ -n "$g" ] && awk -v a="$g" -v c="$bestg" 'BEGIN{exit !(a>c)}'; then
#       bestg=$g; bpx=$px; bpy=$py
#     fi
#   done
# done
# echo "Best geometry at P=$MAXP: ${bpx}x${bpy} ($bestg Gflops)"

# # (b) communication overhead on the best geometry; shrink N by sqrt(2) until overhead >= 25%
# N=$N0
# while [ "$N" -ge 32 ]; do
#   w=$(best "$MAXP" -n "$N" -t "$T" -x "$bpx" -y "$bpy")
#   wo=$(best "$MAXP" -n "$N" -t "$T" -x "$bpx" -y "$bpy" -k)
#   echo "comm,$N,$MAXP,$bpx,$bpy,1,0,$w" >> "$OUT"
#   echo "comm,$N,$MAXP,$bpx,$bpy,1,1,$wo" >> "$OUT"
#   [ -n "$w" ] && [ -n "$wo" ] || break
#   ov=$(awk -v a="$w" -v b="$wo" 'BEGIN{printf "%.3f", 1 - a/b}')
#   echo "  N=$N overhead=$ov"
#   awk -v o="$ov" 'BEGIN{exit !(o >= 0.25)}' && break
#   N=$(awk -v n="$N" 'BEGIN{printf "%d", n/1.4142}')
# done

# # (c) MPI + OpenMP with 1D decomposition: processes x threads = MAXP
# th=1
# while [ "$th" -le "$MAXP" ]; do
#   if [ $((MAXP % th)) -eq 0 ]; then
#     P=$((MAXP / th))
#     g=$(MPIFLAGS="$MPIFLAGS --bind-to none" best "$P" -n "$N0" -t "$T" -y "$P" -o "$th")
#     echo "hybrid,$N0,$P,1,$P,$th,0,$g" >> "$OUT"
#     echo "  hybrid $P procs x $th threads: ${g:-FAILED}"
#   fi
#   th=$((th * 2))
# done

# echo "done -> $OUT   (failed runs, if any, are listed in errors.log)"