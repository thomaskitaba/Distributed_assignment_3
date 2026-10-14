#!/bin/bash
# Performance studies for the assignment. Run on the 16-core machine.
# Output: results.csv (study,N,P,px,py,threads,no_comm,gflops)
RUN="mpirun -n"
OUT=results.csv
echo "study,N,P,px,py,threads,no_comm,gflops" > $OUT
g() { grep "Sustained Gflops" | awk '{print $NF}'; }

# serial baseline
echo "serial,1024,1,1,1,1,0,$(./serial -n 1024 -t 100 | g)" >> $OUT

# (a) strong scaling, N=1024, T=100, all 1D/2D geometries, no OpenMP
for P in 1 2 4 8 16; do
  for py in 1 2 4 8 16; do
    [ $py -le $P ] || continue
    px=$((P / py)); [ $((px * py)) -eq $P ] || continue
    echo "strong,1024,$P,$px,$py,1,0,$($RUN $P ./cardiacsim -n 1024 -t 100 -x $px -y $py | g)" >> $OUT
  done
done

# (b) communication overhead: shrink N by sqrt(2) until overhead >= 25%
P=16; px=4; py=4   # set to the best geometry found in (a)
N=1024
while [ $N -ge 64 ]; do
  with=$($RUN $P ./cardiacsim -n $N -t 100 -x $px -y $py | g)
  without=$($RUN $P ./cardiacsim -n $N -t 100 -x $px -y $py -k | g)
  echo "comm,$N,$P,$px,$py,1,0,$with" >> $OUT
  echo "comm,$N,$P,$px,$py,1,1,$without" >> $OUT
  N=$(python3 -c "print(int($N/1.4142))")
done

# (c) MPI+OpenMP, 1D decomposition, 16 cores total
for cfg in "16 1" "8 2" "4 4" "2 8"; do
  set -- $cfg
  echo "hybrid,1024,$1,1,$1,$2,0,$($RUN $1 ./cardiacsim -n 1024 -t 100 -y $1 -o $2 | g)" >> $OUT
done
echo "done -> $OUT"
