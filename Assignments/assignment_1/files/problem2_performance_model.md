# Problem 2: Performance Model for Parallel Sum (Lecture 4)

**Note on assumptions:** I don't have your specific Lecture 4 slides, so this models
the standard variants of the parallel-sum problem as commonly presented (per-addition
locking vs. local-sum-then-single-lock vs. array-based lock-free combine — the same
pattern the `pthreads_pi.c` example in this course uses). If your lecture defines
versions 2, 3, and 5 differently, let me know the specifics and I'll rework the models.

## Variables

- `S` — sequential execution time
- `T` — number of threads
- `O` — parallelization overhead (fixed, e.g. thread creation/teardown)
- `B` — cost of one barrier synchronization
- `M` — cost of one mutex lock/unlock pair
- `N` — number of elements being summed

## (a) Execution time models

### Version 2 — mutex on every addition

Each of the N additions into the shared sum must acquire the mutex, and since the
lock forces those operations to serialize across all threads, the locking cost scales
with N rather than T:

```
Tparallel(v2) = O + S/T + N·M
```

Because `N·M` doesn't shrink as T grows, this term typically dominates and can make
Version 2 *slower* than the sequential version for anything beyond a couple of
threads — the classic case of synchronization overhead overwhelming the parallel
work.

### Version 3 — local sum, single lock at the end

Each thread accumulates its own partial sum with no locking during the loop, then
takes the mutex exactly once to fold its result into the global sum. Only T lock
operations happen in total, and they serialize:

```
Tparallel(v3) = O + S/T + T·M
```

Since `T ≪ N` in essentially every practical case, this is dramatically cheaper than
Version 2.

### Version 5 — array-based combine (no locking), barrier + thread-0 reduction

Each thread writes its partial sum into its own slot of a shared array (no
contention, since each thread owns a distinct memory location), then all threads
synchronize at a barrier, and thread 0 does the final reduction by summing the array.
The extra work for thread 0 is (T − 1) additions; since the original N-element sum
costs S in total, each addition costs roughly `S/N`, so the combine step costs about
`(T − 1)·(S/N)`:

```
Tparallel(v5) = O + S/T + B + (T − 1)·(S/N)
```

This avoids mutex overhead entirely in exchange for one barrier and a small serial
cleanup step.

## (b) When is Version 3 profitable?

Parallelization is worthwhile when `Tparallel(v3) < Tserial = S`:

```
O + S/T + T·M < S
```

Rearranging to isolate the fixed costs on one side:

```
O + T·M < S · (T − 1) / T
```

**Interpretation:**

- For a **fixed T**, this gives a *minimum problem size* `S` below which the
  sequential version wins — small sums aren't worth spawning threads for, since `O`
  and `T·M` are fixed costs that a tiny `S` can't amortize.
- For a **fixed S**, this gives a *maximum useful thread count*. As T grows,
  `(T − 1)/T → 1` (diminishing returns — you can't beat unit efficiency), while
  `T·M` grows linearly and eventually outpaces the shrinking benefit. Past that
  crossover point, adding more threads makes Version 3 slower, not faster — the
  same diminishing/negative-returns pattern seen numerically in Problem 1 when p
  got large relative to n.

So Version 3 pays off exactly when the per-element work `S/N` is large enough, or N
is large enough, that the savings from splitting work across T threads outweigh the
fixed overhead `O` plus the small T-proportional locking cost `T·M`.
