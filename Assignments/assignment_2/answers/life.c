/* **********************************************************
 * Parallel Game of Life -- Pthreads version
 *
 * Task parallelism : one dedicated "plotter" thread handles the
 *                     gnuplot display while a pool of "worker"
 *                     threads compute the cell updates.
 * Data parallelism  : the interior rows of the mesh are divided
 *                     (as evenly as possible) among the worker
 *                     threads using a 1-D (row-wise) decomposition.
 *
 * Synchronisation:
 *   - pthread_barrier_t is used twice per generation among the
 *     worker threads:
 *       (1) after every worker has finished updating its rows,
 *           so the designated "leader" (worker 0) can safely swap
 *           the currWorld/nextWorld pointers and total the
 *           population.
 *       (2) after the leader has finished that serial section, so
 *           every worker sees the updated currWorld pointer and
 *           the "keep_running" flag before starting the next
 *           generation (or exiting).
 *   - A small producer/consumer hand-off (mutex + 2 condition
 *     variables) passes a *copy* of the board to the plotter
 *     thread.  Because the plotter works off its own private
 *     copy (plotBoard), it can render generation t while the
 *     worker threads are already computing generation t+1 --
 *     there is no data race between the compute threads and the
 *     plotter.
 *
 * Based on the serial life.c by Urvashi R.V., S. Baden, P. Cicotti,
 * D. Unat.
 *************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#define MATCH(s) (!strcmp(argv[ac], (s)))

int MeshPlot(int t, int m, int n, char **mesh);

double real_rand();
int seed_rand(long sd);
double getTime();
extern FILE *gnu;

/* ------------------------------------------------------------------ */
/* Global, shared simulation state                                     */
/* ------------------------------------------------------------------ */
static char **currWorld = NULL, **nextWorld = NULL;
static int nx = 100;   /* mesh points in x, including 2 ghost cells   */
static int ny = 100;   /* mesh points in y, including 2 ghost cells   */
static int maxiter = 200;
static int population = 0;   /* population of currWorld                */
static int numthreads = 1;   /* number of COMPUTE (worker) threads     */
static int disable_display = 0;
static int s_step = 0;
static int t = 0;            /* current generation index               */
static int keep_running = 1;
static const char *dumpFile = NULL; /* optional: dump final board for diff-testing */

static pthread_barrier_t barrier;

/* ------------------------------------------------------------------ */
/* Plotter hand-off (producer/consumer, capacity 1)                    */
/* ------------------------------------------------------------------ */
static char **plotBoard = NULL;
static int plot_t = 0;
static int plot_has_data = 0;
static int plot_done = 0;
static pthread_mutex_t plot_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  plot_cond_produced = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  plot_cond_consumed = PTHREAD_COND_INITIALIZER;

/* Push a snapshot of `board` (labelled generation `gen`) to the
 * plotter thread.  Blocks only if the plotter hasn't consumed the
 * previous frame yet. */
static void producePlotFrame(char **board, int gen)
{
    int i;
    pthread_mutex_lock(&plot_mutex);
    while (plot_has_data)
        pthread_cond_wait(&plot_cond_consumed, &plot_mutex);
    for (i = 0; i < nx; i++)
        memcpy(plotBoard[i], board[i], ny);
    plot_t = gen;
    plot_has_data = 1;
    pthread_cond_signal(&plot_cond_produced);
    pthread_mutex_unlock(&plot_mutex);
}

static void producePlotDone(void)
{
    pthread_mutex_lock(&plot_mutex);
    plot_done = 1;
    pthread_cond_signal(&plot_cond_produced);
    pthread_mutex_unlock(&plot_mutex);
}

static void *plotterThreadFunc(void *arg)
{
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&plot_mutex);
        while (!plot_has_data && !plot_done)
            pthread_cond_wait(&plot_cond_produced, &plot_mutex);

        if (!plot_has_data && plot_done) {
            pthread_mutex_unlock(&plot_mutex);
            break;
        }

        /* MeshPlot only touches plotBoard, which nobody else writes
         * to while plot_has_data == 1, so it's safe to call it while
         * holding the lock -- the producer will simply block on
         * plot_cond_consumed until we're done. */
        MeshPlot(plot_t, nx, ny, plotBoard);

        plot_has_data = 0;
        pthread_cond_signal(&plot_cond_consumed);
        pthread_mutex_unlock(&plot_mutex);
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Worker (compute) threads                                            */
/* ------------------------------------------------------------------ */
typedef struct {
    int id;
    int start_row, end_row; /* inclusive range of interior rows to own */
    int local_pop;
} WorkerArg;

static WorkerArg *workers = NULL;

static void *workerThreadFunc(void *argp)
{
    WorkerArg *w = (WorkerArg *)argp;
    int i, j;

    for (;;) {
        /* ---- compute this worker's rows of nextWorld ---- */
        w->local_pop = 0;
        for (i = w->start_row; i <= w->end_row; i++) {
            for (j = 1; j < ny - 1; j++) {
                int nn = currWorld[i + 1][j] + currWorld[i - 1][j] +
                          currWorld[i][j + 1] + currWorld[i][j - 1] +
                          currWorld[i + 1][j + 1] + currWorld[i - 1][j - 1] +
                          currWorld[i - 1][j + 1] + currWorld[i + 1][j - 1];

                nextWorld[i][j] = currWorld[i][j] ? (nn == 2 || nn == 3) : (nn == 3);
                w->local_pop += nextWorld[i][j];
            }
        }

        pthread_barrier_wait(&barrier); /* (1) all updates finished */

        if (w->id == 0) {
            char **tmp;
            int k, newpop = 0;

            /* Pointer swap : nextWorld <-> currWorld */
            tmp = nextWorld;
            nextWorld = currWorld;
            currWorld = tmp;

            for (k = 0; k < numthreads; k++)
                newpop += workers[k].local_pop;
            population = newpop;

            if (!disable_display)
                producePlotFrame(currWorld, t);

            if (s_step) {
                printf("Finished with step %d\n", t);
                printf("Press enter to continue.\n");
                getchar();
            }

            t++;
            keep_running = (t < maxiter) && (population > 0);
        }

        pthread_barrier_wait(&barrier); /* (2) leader's serial section done */

        if (!keep_running)
            break;
    }
    return NULL;
}

/* Divide the `n` interior rows [1 .. n] as evenly as possible among
 * `nthreads` workers.  Handles the case where nthreads doesn't
 * divide n. */
static void assignRows(int n, int nthreads)
{
    int base = n / nthreads;
    int rem = n % nthreads;
    int row = 1;
    int k;

    for (k = 0; k < nthreads; k++) {
        int count = base + (k < rem ? 1 : 0);
        workers[k].id = k;
        if (count > 0) {
            workers[k].start_row = row;
            workers[k].end_row = row + count - 1;
        } else {
            /* more threads than rows: give this one an empty range */
            workers[k].start_row = 1;
            workers[k].end_row = 0;
        }
        row += count;
    }
}

int main(int argc, char **argv)
{
    int i, j, ac;
    float prob = 0.5;
    long seedVal = 0;
    int game = 0;
    pthread_t plotterThread;
    pthread_t *workerThreads;

    for (ac = 1; ac < argc; ac++) {
        if (MATCH("-n"))       { nx = atoi(argv[++ac]); }
        else if (MATCH("-i"))  { maxiter = atoi(argv[++ac]); }
        else if (MATCH("-t"))  { numthreads = atoi(argv[++ac]); }
        else if (MATCH("-p"))  { prob = atof(argv[++ac]); }
        else if (MATCH("-s"))  { seedVal = atof(argv[++ac]); }
        else if (MATCH("-step")) { s_step = 1; }
        else if (MATCH("-d"))  { disable_display = 1; }
        else if (MATCH("-g"))  { game = atoi(argv[++ac]); }
        else if (MATCH("-dump")) { dumpFile = argv[++ac]; }
        else {
            printf("Usage: %s [-n <meshpoints>] [-i <iterations>] [-s seed] [-p prob] [-t numthreads] [-step] [-g <game #>] [-d]\n", argv[0]);
            return (-1);
        }
    }

    if (numthreads < 1) numthreads = 1;

    int rs = seed_rand(seedVal);

    nx = nx + 2;
    ny = nx;

    /* --- master thread allocates & initialises the board (per spec) --- */
    currWorld = (char **)malloc(sizeof(char *) * nx + sizeof(char) * nx * ny);
    for (i = 0; i < nx; i++)
        currWorld[i] = (char *)(currWorld + nx) + i * ny;

    nextWorld = (char **)malloc(sizeof(char *) * nx + sizeof(char) * nx * ny);
    for (i = 0; i < nx; i++)
        nextWorld[i] = (char *)(nextWorld + nx) + i * ny;

    plotBoard = (char **)malloc(sizeof(char *) * nx + sizeof(char) * nx * ny);
    for (i = 0; i < nx; i++)
        plotBoard[i] = (char *)(plotBoard + nx) + i * ny;

    for (i = 0; i < nx; i++) {
        currWorld[i][0] = 0; currWorld[i][ny - 1] = 0;
        nextWorld[i][0] = 0; nextWorld[i][ny - 1] = 0;
    }
    for (i = 0; i < ny; i++) {
        currWorld[0][i] = 0; currWorld[nx - 1][i] = 0;
        nextWorld[0][i] = 0; nextWorld[nx - 1][i] = 0;
    }

    if (game == 0) {
        for (i = 1; i < nx - 1; i++)
            for (j = 1; j < ny - 1; j++) {
                currWorld[i][j] = (real_rand() < prob);
                population += currWorld[i][j];
            }
    } else if (game == 1) {
        printf("2x2 Block, still life\n");
        int nx2 = nx / 2, ny2 = ny / 2;
        currWorld[nx2 + 1][ny2 + 1] = currWorld[nx2][ny2 + 1] =
            currWorld[nx2 + 1][ny2] = currWorld[nx2][ny2] = 1;
        population = 4;
    } else if (game == 2) {
        printf("Glider (spaceship)\n");
        int gx = 2, gy = 2; /* anchor near top-left, inside ghost border */
        currWorld[gx][gy + 1] = 1;
        currWorld[gx + 1][gy + 2] = 1;
        currWorld[gx + 2][gy] = 1;
        currWorld[gx + 2][gy + 1] = 1;
        currWorld[gx + 2][gy + 2] = 1;
        population = 5;
    } else {
        printf("Unknown game %d\n", game);
        exit(-1);
    }

    printf("probability: %f\n", prob);
    printf("Random # generator seed: %d\n", rs);
    printf("Compute threads: %d\n", numthreads);

    if (!disable_display)
        MeshPlot(0, nx, ny, currWorld);

    /* --- set up worker row assignment --- */
    workers = (WorkerArg *)malloc(sizeof(WorkerArg) * numthreads);
    assignRows(nx - 2, numthreads);

    keep_running = (maxiter > 0) && (population > 0);

    pthread_barrier_init(&barrier, NULL, numthreads);

    workerThreads = (pthread_t *)malloc(sizeof(pthread_t) * numthreads);

    double t0 = getTime();

    if (!disable_display)
        pthread_create(&plotterThread, NULL, plotterThreadFunc, NULL);

    if (keep_running) {
        for (i = 0; i < numthreads; i++)
            pthread_create(&workerThreads[i], NULL, workerThreadFunc, &workers[i]);
        for (i = 0; i < numthreads; i++)
            pthread_join(workerThreads[i], NULL);
    }

    double t1 = getTime();

    if (!disable_display) {
        producePlotDone();
        pthread_join(plotterThread, NULL);
    }

    printf("Running time for the iterations: %f sec.\n", t1 - t0);

    if (dumpFile != NULL) {
        FILE *fp = fopen(dumpFile, "w");
        if (fp != NULL) {
            for (i = 1; i < nx - 1; i++) {
                for (j = 1; j < ny - 1; j++)
                    fputc(currWorld[i][j] ? '1' : '0', fp);
                fputc('\n', fp);
            }
            fclose(fp);
        }
    }

    printf("Press enter to end.\n");
    getchar();

    if (gnu != NULL)
        pclose(gnu);

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&plot_mutex);
    pthread_cond_destroy(&plot_cond_produced);
    pthread_cond_destroy(&plot_cond_consumed);

    free(workers);
    free(workerThreads);
    free(plotBoard);
    free(nextWorld);
    free(currWorld);

    return (0);
}
