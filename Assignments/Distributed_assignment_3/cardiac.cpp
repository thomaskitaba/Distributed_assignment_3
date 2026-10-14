/*
 * Aliev-Panfilov cardiac electrophysiology simulator
 * MPI (1D / 2D processor geometry) + OpenMP hybrid version.
 *
 * Based on the template by Xing Cai, Scott B. Baden and Didem Unat.
 *
 * Optimizations (see report):
 *   - PDE + two ODE sweeps fused into one pass over memory
 *   - halo exchange with MPI_Sendrecv, MPI_PROC_NULL at domain edges
 *     (no per-rank special cases), columns packed into contiguous buffers
 *   - buffers allocated once, not on every timestep
 *   - OpenMP over rows with first-touch initialisation
 *   - single MPI_Reduce for the statistics, no barrier per timestep
 */
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h>

using namespace std;

static double getTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + 1.0e-6 * (double)tv.tv_usec;
}

// contiguous 2D array with row pointers, m rows x n columns
static double **alloc2D(int m, int n) {
    double **E = (double **)malloc(sizeof(double *) * m + sizeof(double) * (size_t)m * n);
    assert(E);
    double *data = (double *)(E + m);
    for (int j = 0; j < m; j++) E[j] = data + (size_t)j * n;
    return E;
}

// optional gnuplot output (only meaningful on a single process)
static void splot(double **E, double T, int niter, int m, int n) {
    static FILE *gp = NULL;
    if (!gp) gp = popen("gnuplot -persist", "w");
    if (!gp) return;
    fprintf(gp, "set title 't=%.2f niter=%d'\nset size square\nset key off\n", T, niter);
    fprintf(gp, "set pm3d map\nsplot [0:%d][0:%d][-1:1] '-'\n", n - 1, m - 1);
    for (int j = 0; j < m; j++) {
        for (int i = 0; i < n; i++) fprintf(gp, "%d %d %f\n", i, j, E[j][i]);
        fprintf(gp, "\n");
    }
    fprintf(gp, "e\n");
    fflush(gp);
}

static void usage(const char *p) {
    printf("Usage: %s [-t T] [-n N] [-x px] [-y py] [-o threads] [-p plot_freq] [-k]\n"
           "  -t  duration of simulation\n  -n  mesh points in x and y\n"
           "  -x  processor geometry along x   -y  processor geometry along y\n"
           "  -o  OpenMP threads per process    -p  plot frequency (1 process only)\n"
           "  -k  disable communication (for the overhead study)\n", p);
}

static void cmdLine(int argc, char **argv, double &T, int &n, int &px, int &py,
                    int &plot_freq, int &no_comm, int &nthreads) {
    int c;
    while ((c = getopt(argc, argv, "t:n:x:y:o:p:kh")) != -1) {
        switch (c) {
        case 't': T = atof(optarg); break;
        case 'n': n = atoi(optarg); break;
        case 'x': px = atoi(optarg); break;
        case 'y': py = atoi(optarg); break;
        case 'o': nthreads = atoi(optarg); break;
        case 'p': plot_freq = atoi(optarg); break;
        case 'k': no_comm = 1; break;
        default: usage(argv[0]); MPI_Finalize(); exit(0);
        }
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int nprocs, myrank;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    // Constants - these definitions shouldn't change
    const double a = 0.1, b = 0.1, kk = 8.0, M1 = 0.07, M2 = 0.3, epsilon = 0.01, d = 5e-5;

    double T = 1000.0;
    int n = 200, px = 0, py = 0, plot_freq = 0, no_comm = 0, nthreads = 1;
    cmdLine(argc, argv, T, n, px, py, plot_freq, no_comm, nthreads);
    const int N = n;  // square mesh

    // processor geometry: default is 1D (px = 1, py = nprocs)
    if (px <= 0 && py <= 0) { px = 1; py = nprocs; }
    else if (px <= 0) px = nprocs / py;
    else if (py <= 0) py = nprocs / px;
    if (px * py != nprocs) {
        if (myrank == 0) fprintf(stderr, "ERROR: px*py = %d but %d processes were started\n", px * py, nprocs);
        MPI_Finalize();
        return 1;
    }
    if (nthreads < 1) nthreads = 1;
    omp_set_num_threads(nthreads);

    // ---- domain decomposition: last row/column of ranks gets the remainder ----
    const int rowID = myrank / px, colID = myrank % px;
    const int rinterval = N / py, cinterval = N / px;
    const int m = rinterval + (rowID == py - 1 ? N % py : 0);  // local rows
    const int nn = cinterval + (colID == px - 1 ? N % px : 0); // local columns
    const int rbeg = rowID * rinterval + 1;                    // global index of local row 1
    const int cbeg = colID * cinterval + 1;                    // global index of local col 1

    const int north = (rowID > 0)      ? myrank - px : MPI_PROC_NULL;
    const int south = (rowID < py - 1) ? myrank + px : MPI_PROC_NULL;
    const int west  = (colID > 0)      ? myrank - 1  : MPI_PROC_NULL;
    const int east  = (colID < px - 1) ? myrank + 1  : MPI_PROC_NULL;

    double **E      = alloc2D(m + 2, nn + 2);
    double **E_prev = alloc2D(m + 2, nn + 2);
    double **R      = alloc2D(m + 2, nn + 2);

    // first-touch initialisation with the same row distribution as the compute loop
    #pragma omp parallel for schedule(static)
    for (int j = 0; j < m + 2; j++)
        for (int i = 0; i < nn + 2; i++) {
            E[j][i] = 0.0;
            R[j][i] = 0.0;
            E_prev[j][i] = 0.0;
        }

    // initial conditions (identical to the serial code, using global indices)
    #pragma omp parallel for schedule(static)
    for (int j = 1; j <= m; j++) {
        const int grow = rbeg + j - 1;
        for (int i = 1; i <= nn; i++) {
            const int gcol = cbeg + i - 1;
            if (gcol >= N / 2 + 1) E_prev[j][i] = 1.0;
            if (grow >= N / 2 + 1) R[j][i] = 1.0;
        }
    }

    // time step (same as serial)
    const double dx = 1.0 / N;
    const double rp = kk * (b + 1) * (b + 1) / 4;
    const double dte = (dx * dx) / (d * 4 + ((dx * dx)) * (rp + kk));
    const double dtr = 1 / (epsilon + ((M1 / M2) * rp));
    const double dt = (dte < dtr) ? 0.95 * dte : 0.95 * dtr;
    const double alpha = d * dt / (dx * dx);

    if (myrank == 0) {
        cout << "Grid Size       : " << N << endl;
        cout << "Duration of Sim : " << T << endl;
        cout << "Time step dt    : " << dt << endl;
        cout << "Process geometry: " << px << " x " << py << endl;
        cout << "Threads/process : " << nthreads << endl;
        cout << "Communication   : " << (no_comm ? "DISABLED" : "enabled") << endl << endl;
    }

    // packing buffers for the (non-contiguous) west/east columns - allocated once
    vector<double> sendW(m), sendE(m), recvW(m), recvE(m);

    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = getTime();
    double t = 0.0;
    int niter = 0;

    while (t < T) {
        t += dt;
        niter++;

        // ---------- mirror boundaries on the physical edges of the global domain ----------
        if (north == MPI_PROC_NULL) {
            #pragma omp parallel for schedule(static)
            for (int i = 1; i <= nn; i++) E_prev[0][i] = E_prev[2][i];
        }
        if (south == MPI_PROC_NULL) {
            #pragma omp parallel for schedule(static)
            for (int i = 1; i <= nn; i++) E_prev[m + 1][i] = E_prev[m - 1][i];
        }
        if (west == MPI_PROC_NULL) {
            #pragma omp parallel for schedule(static)
            for (int j = 1; j <= m; j++) E_prev[j][0] = E_prev[j][2];
        }
        if (east == MPI_PROC_NULL) {
            #pragma omp parallel for schedule(static)
            for (int j = 1; j <= m; j++) E_prev[j][nn + 1] = E_prev[j][nn - 1];
        }

        // ---------- ghost cell exchange ----------
        if (!no_comm) {
            // rows are contiguous in memory: send directly from / receive directly into the array
            if (py > 1) {
                MPI_Sendrecv(&E_prev[1][1], nn, MPI_DOUBLE, north, 0,
                             &E_prev[m + 1][1], nn, MPI_DOUBLE, south, 0,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Sendrecv(&E_prev[m][1], nn, MPI_DOUBLE, south, 1,
                             &E_prev[0][1], nn, MPI_DOUBLE, north, 1,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            // columns are strided: pack, exchange, unpack
            if (px > 1) {
                if (west != MPI_PROC_NULL)
                    for (int j = 1; j <= m; j++) sendW[j - 1] = E_prev[j][1];
                if (east != MPI_PROC_NULL)
                    for (int j = 1; j <= m; j++) sendE[j - 1] = E_prev[j][nn];

                MPI_Sendrecv(sendW.data(), m, MPI_DOUBLE, west, 2,
                             recvE.data(), m, MPI_DOUBLE, east, 2,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Sendrecv(sendE.data(), m, MPI_DOUBLE, east, 3,
                             recvW.data(), m, MPI_DOUBLE, west, 3,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if (east != MPI_PROC_NULL)
                    for (int j = 1; j <= m; j++) E_prev[j][nn + 1] = recvE[j - 1];
                if (west != MPI_PROC_NULL)
                    for (int j = 1; j <= m; j++) E_prev[j][0] = recvW[j - 1];
            }
        }

        // ---------- fused PDE + ODE update ----------
        #pragma omp parallel for schedule(static)
        for (int j = 1; j <= m; j++) {
            const double *ep_c = E_prev[j];
            const double *ep_n = E_prev[j - 1];
            const double *ep_s = E_prev[j + 1];
            double *e_row = E[j];
            double *r_row = R[j];
            #pragma omp simd
            for (int i = 1; i <= nn; i++) {
                double e = ep_c[i] + alpha * (ep_c[i + 1] + ep_c[i - 1] - 4 * ep_c[i] + ep_s[i] + ep_n[i]);
                const double r = r_row[i];
                e = e - dt * (kk * e * (e - a) * (e - 1) + e * r);
                r_row[i] = r + dt * (epsilon + M1 * r / (e + M2)) * (-r - kk * e * (e - b - 1));
                e_row[i] = e;
            }
        }

        double **tmp = E; E = E_prev; E_prev = tmp;

        if (plot_freq && myrank == 0 && nprocs == 1) {
            int k = (int)(t / plot_freq);
            if ((t - k * plot_freq) < dt) splot(E_prev, t, niter, m + 2, nn + 2);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    const double time_elapsed = getTime() - t0;

    const double Gflops = (double)(niter * (1E-9 * N * N) * 28.0) / time_elapsed;
    const double BW = (double)(niter * 1E-9 * (N * (double)N * sizeof(double) * 4.0)) / time_elapsed;

    // ---------- statistics (final solution is in E_prev after the swap) ----------
    double lmax = -1.0, lsq = 0.0;
    #pragma omp parallel for reduction(+ : lsq) reduction(max : lmax)
    for (int j = 1; j <= m; j++)
        for (int i = 1; i <= nn; i++) {
            const double v = E_prev[j][i];
            lsq += v * v;
            if (v > lmax) lmax = v;
        }
    double gmax, gsq;
    MPI_Reduce(&lmax, &gmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&lsq, &gsq, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (myrank == 0) {
        cout << "Number of Iterations        : " << niter << endl;
        cout << "Elapsed Time (sec)          : " << time_elapsed << endl;
        cout << "Sustained Gflops Rate       : " << Gflops << endl;
        cout << "Sustained Bandwidth (GB/sec): " << BW << endl << endl;
        cout << "Max: " << gmax << " L2norm : " << sqrt(gsq / ((double)N * N)) << endl;
    }

    free(E); free(E_prev); free(R);
    MPI_Finalize();
    return 0;
}
