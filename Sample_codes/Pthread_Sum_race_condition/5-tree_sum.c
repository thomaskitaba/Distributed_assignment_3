#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/* ============================================================================
 * TREE-BASED PARALLEL REDUCTION
 * ============================================================================
 * CONCEPT:
 * Instead of having every worker thread hand its result to a single "master"
 * thread (which then has to add t results one by one -> O(t) serial work),
 * we arrange the additions as a BINARY TREE:
 *
 *      leaf sums:   [8] [19] [7] [15] [7] [13] [12] [14]     <- t = 8 leaves
 *                     \  /     \  /     \  /      \  /
 * level 0 (4 adds):    27       22       20        26         <- t/2 workers
 *                        \      /           \      /
 * level 1 (2 adds):        49                  46              <- t/4 workers
 *                             \                /
 * level 2 (1 add):                  95                          <- 1 worker
 *
 * Each level can run its additions IN PARALLEL (different threads, disjoint
 * data), and the number of levels needed to collapse t values down to 1 is
 * log2(t). So instead of t-1 serial additions by a single master thread, we
 * only need log2(t) *sequential* steps (each step itself parallel).
 *
 * For t = 1000 threads: a serial master would do 999 adds one after another.
 * A tree only needs ceil(log2(1000)) = 10 sequential steps. That's the whole
 * performance win described in the slide.
 * ============================================================================
 */

#define NUMBEROFITEMS   24
#define NUMBEROFTHREADS 8   // must be a power of 2 so the tree divides evenly

int values[] = {
    1, 4, 3,
    9, 2, 8,
    5, 1, 1,
    6, 2, 7,
    2, 5, 0,
    4, 1, 8,
    6, 5, 1,
    2, 3, 9
};

// How many array elements each leaf thread is responsible for.
// e.g. 24 items / 8 threads = 3 items per thread.
int tasks = NUMBEROFITEMS / NUMBEROFTHREADS;

/* ----------------------------------------------------------------------
 * CONCEPT: Why two arrays instead of one?
 * ----------------------------------------------------------------------
 * partial_sum[] always holds the CURRENT level's values (the inputs that
 * this round of threads will read).
 * next_level[]  always holds the NEXT level's values (the outputs this
 * round of threads will produce).
 *
 * This separation is what makes the tree reduction race-free WITHOUT a
 * mutex. If we tried to read and write the same array in the same round
 * (e.g. writing pair (0,1)'s result into slot 0 while another thread is
 * simultaneously reading slot 1 in order to compute pair (2,3)'s partner
 * assignment), we would have a classic reader/writer race: one thread
 * could see a half-updated array. By keeping "this round's inputs" and
 * "this round's outputs" in physically separate memory, no thread ever
 * touches a memory location another thread is touching at the same time
 * -> no synchronization primitive (mutex/lock) is required at all.
 * The only synchronization we need is a BARRIER between rounds, and
 * pthread_join() gives us that for free: main() doesn't move on to the
 * next level (or copy next_level back into partial_sum) until every
 * thread from the current level has fully finished and exited.
 * ---------------------------------------------------------------------- */
int partial_sum[NUMBEROFTHREADS]; // current level's data (read from here)
int next_level[NUMBEROFTHREADS];  // next level's data    (write to here)

int compute_next_value(int values[], int idx);

/* ============================================================================
 * LEAF LEVEL: each of the NUMBEROFTHREADS threads sums its own slice of the
 * input array. This is the "embarrassingly parallel" part of the algorithm:
 * the work has already been statically partitioned so that no two threads
 * ever look at the same input element, and each thread writes its result to
 * its OWN, UNIQUE slot (partial_sum[id]).
 *
 * CONCEPT: Why no mutex here?
 * A mutex is only needed when multiple threads might read-modify-write the
 * SAME memory location concurrently (like the original code's shared global
 * `sum`, where every thread did `sum += ...` on the exact same variable).
 * Here, thread i only ever touches partial_sum[i] - a private lane, not a
 * shared accumulator. Different memory addresses = no race, by construction,
 * regardless of the order or timing in which the OS schedules the threads.
 * ============================================================================
 */
typedef struct {
    int thread_id;
} leaf_arg_t;

void *leaf_calculate(void *arg) {
    leaf_arg_t *a = (leaf_arg_t *)arg;
    int id = a->thread_id;
    int start = id * tasks;
    int local_sum = 0;

    printf("[leaf %d] start index: %d - end index: %d\n", id, start, start + tasks - 1);

    // Sequentially sum this thread's private chunk of the array.
    // No locking needed: 'local_sum' is a stack variable, private to this
    // thread, and 'values' is only ever read (never written) by anyone.
    for (int j = start; j < start + tasks; j++) {
        local_sum += compute_next_value(values, j);
    }

    // Publish the result to this thread's own, exclusive slot.
    partial_sum[id] = local_sum;
    printf("[leaf %d] local sum = %d\n", id, local_sum);
    return NULL;
}

/* ============================================================================
 * INTERNAL TREE LEVELS: combine pairs of values from the current level into
 * single values for the next level. Called repeatedly, once per tree level,
 * with a fresh batch of threads each time (fewer threads at each level, since
 * the number of live values halves every round).
 *
 * CONCEPT: This is the actual "tree" from the diagram. At level 0 we combine
 * 8 leaves into 4 values; at level 1, 4 values into 2; at level 2, 2 values
 * into the final global sum. Each combine() call is one node (one colored
 * cube) in the picture; each level is one row of cubes.
 * ============================================================================
 */
typedef struct {
    int left_idx;   // index into partial_sum[] of this pair's first value
    int right_idx;  // index into partial_sum[] of this pair's second value
    int dest_idx;   // index into next_level[] where the sum is stored
    int level;      // which tree level we're on (for logging only)
} combine_arg_t;


void *combine(void *arg) {
    combine_arg_t *a = (combine_arg_t *)arg;

    // READ from partial_sum (this level's inputs) ...
    // ... WRITE to next_level (this level's outputs).
    // Because reads and writes target two different arrays, this thread's
    // memory accesses never collide with any other combine thread running
    // at the same time in this same round, even though the *numeric*
    // index ranges overlap (e.g. dest_idx values 0,1,2,3 look like they
    // "reuse" left_idx/right_idx values 0,1,2,3 - but they live in a
    // different array, so there's no actual collision).
    int result = partial_sum[a->left_idx] + partial_sum[a->right_idx];

    printf("[level %d] combine partial_sum[%d] (%d) + partial_sum[%d] (%d) -> next_level[%d] = %d\n",
           a->level, a->left_idx, partial_sum[a->left_idx],
           a->right_idx, partial_sum[a->right_idx], a->dest_idx, result);

    next_level[a->dest_idx] = result;
    return NULL;
}

int main() {
    pthread_t leaf_threads[NUMBEROFTHREADS];
    leaf_arg_t leaf_args[NUMBEROFTHREADS];

    /* ------------------------------------------------------------------
     * STEP 1: Fan out - create all leaf threads so they run concurrently.
     * ------------------------------------------------------------------ */
    for (int i = 0; i < NUMBEROFTHREADS; i++) {
        leaf_args[i].thread_id = i;
        pthread_create(&leaf_threads[i], NULL, leaf_calculate, &leaf_args[i]);
    }

    /* ------------------------------------------------------------------
     * CONCEPT: The join loop below is a BARRIER. main() will block here
     * until every single leaf thread has finished writing its result into
     * partial_sum[]. This guarantees that by the time we start the tree
     * reduction below, partial_sum[] is fully and safely populated -
     * there is no possibility of a combine() thread reading a leaf slot
     * that hasn't been written yet.
     * ------------------------------------------------------------------ */
    for (int i = 0; i < NUMBEROFTHREADS; i++) {
        pthread_join(leaf_threads[i], NULL);
    }

    int num_levels = 0;
    for (int n = NUMBEROFTHREADS; n > 1; n /= 2) num_levels++;
    printf("\n--- Leaf sums done, starting tree reduction (log2(%d) = %d levels) ---\n\n",
           NUMBEROFTHREADS, num_levels);

    /* ------------------------------------------------------------------
     * STEP 2: Tree reduction - log2(NUMBEROFTHREADS) sequential rounds,
     * each round itself fully parallel.
     *
     * CONCEPT: 'active' tracks how many "live" values still need to be
     * summed together. It starts at NUMBEROFTHREADS (all leaves) and
     * HALVES every round, because each round pairs values up and produces
     * one output per pair. The loop runs until only 1 value remains -
     * that value IS the global sum. The number of iterations this while
     * loop performs is exactly log2(NUMBEROFTHREADS), which is the whole
     * point of the exercise: log(t) sequential steps instead of t serial
     * additions by a single master thread.
     * ------------------------------------------------------------------ */
    int active = NUMBEROFTHREADS;
    int level = 0;

    while (active > 1) {
        int pairs = active / 2;                 // values produced this round
        pthread_t combine_threads[pairs];
        combine_arg_t combine_args[pairs];

        // Fan out: spawn one thread per pair, all running concurrently.
        // Pair p reads slots (2p, 2p+1) from partial_sum and writes its
        // result to slot p of next_level. Every thread's (left_idx,
        // right_idx) pair is disjoint from every other thread's pair, and
        // every thread's dest_idx is likewise unique - so, just like the
        // leaf level, there is no shared-memory collision and no mutex
        // is required.
        for (int p = 0; p < pairs; p++) {
            int left_idx  = p * 2;
            int right_idx = p * 2 + 1;

            combine_args[p].left_idx  = left_idx;
            combine_args[p].right_idx = right_idx;
            combine_args[p].dest_idx  = p;
            combine_args[p].level     = level;

            pthread_create(&combine_threads[p], NULL, combine, &combine_args[p]);
        }

        // Barrier again: wait for every thread in this round to finish
        // before touching next_level[] ourselves. This is what makes it
        // safe to treat next_level[] as "done" immediately afterward.
        for (int p = 0; p < pairs; p++) {
            pthread_join(combine_threads[p], NULL);
        }

        /* ----------------------------------------------------------
         * CONCEPT: Promote next_level -> partial_sum for the next
         * round. This copy happens back in main() - single-threaded,
         * after the barrier above - so it can never race with anything.
         * After this, partial_sum[0..pairs-1] holds the current level's
         * results and is ready to be read as INPUT by the next round of
         * combine threads.
         * ---------------------------------------------------------- */
        for (int p = 0; p < pairs; p++) {
            partial_sum[p] = next_level[p];
        }

        active = pairs;   // half as many live values remain
        level++;
    }

    // After the loop, 'active' == 1, meaning the entire tree has collapsed
    // down to a single value sitting in partial_sum[0]: the global sum.
    printf("\nExpected Sum: %d\n", 95);
    printf("Global Sum using tree reduction: %d\n", partial_sum[0]);

    return 0;
}

// Simulates a "slow" per-element computation (e.g. some real workload).
// The usleep() forces the OS scheduler to actually context-switch between
// threads instead of letting one thread finish instantly before another
// even starts - which is what makes parallelism (and any race conditions)
// visible/observable in practice rather than hidden by sheer luck/timing.
int compute_next_value(int values[], int idx) {
    usleep(54000);
    return values[idx];
}