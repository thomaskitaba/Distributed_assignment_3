/* Serial reference implementation of the Aliev-Panfilov simulator (as provided). */
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
using namespace std;

static double getTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + 1.0e-6 * (double)tv.tv_usec;
}
static double **alloc2D(int m, int n) {
    double **E = (double **)malloc(sizeof(double *) * m + sizeof(double) * (size_t)m * n);
    for (int j = 0; j < m; j++) E[j] = (double *)(E + m) + (size_t)j * n;
    return E;
}

int main(int argc, char **argv) {
    const double a = 0.1, b = 0.1, kk = 8.0, M1 = 0.07, M2 = 0.3, epsilon = 0.01, d = 5e-5;
    double T = 1000.0;
    int n = 200, c;
    while ((c = getopt(argc, argv, "t:n:")) != -1) {
        if (c == 't') T = atof(optarg);
        else if (c == 'n') n = atoi(optarg);
    }
    int m = n;
    double **E = alloc2D(m + 2, n + 2), **E_prev = alloc2D(m + 2, n + 2), **R = alloc2D(m + 2, n + 2);
    for (int j = 0; j < m + 2; j++)
        for (int i = 0; i < n + 2; i++) E[j][i] = E_prev[j][i] = R[j][i] = 0.0;
    for (int j = 1; j <= m; j++)
        for (int i = n / 2 + 1; i <= n; i++) E_prev[j][i] = 1.0;
    for (int j = m / 2 + 1; j <= m; j++)
        for (int i = 1; i <= n; i++) R[j][i] = 1.0;

    double dx = 1.0 / n;
    double rp = kk * (b + 1) * (b + 1) / 4;
    double dte = (dx * dx) / (d * 4 + ((dx * dx)) * (rp + kk));
    double dtr = 1 / (epsilon + ((M1 / M2) * rp));
    double dt = (dte < dtr) ? 0.95 * dte : 0.95 * dtr;
    double alpha = d * dt / (dx * dx);

    double t0 = getTime(), t = 0.0;
    int niter = 0;
    while (t < T) {
        t += dt;
        niter++;
        for (int i = 1; i <= n; i++) E_prev[0][i] = E_prev[2][i];
        for (int i = 1; i <= n; i++) E_prev[m + 1][i] = E_prev[m - 1][i];
        for (int j = 1; j <= m; j++) E_prev[j][0] = E_prev[j][2];
        for (int j = 1; j <= m; j++) E_prev[j][n + 1] = E_prev[j][n - 1];

        for (int j = 1; j <= m; j++)
            for (int i = 1; i <= n; i++)
                E[j][i] = E_prev[j][i] + alpha * (E_prev[j][i + 1] + E_prev[j][i - 1] - 4 * E_prev[j][i] + E_prev[j + 1][i] + E_prev[j - 1][i]);
        for (int j = 1; j <= m; j++)
            for (int i = 1; i <= n; i++)
                E[j][i] = E[j][i] - dt * (kk * E[j][i] * (E[j][i] - a) * (E[j][i] - 1) + E[j][i] * R[j][i]);
        for (int j = 1; j <= m; j++)
            for (int i = 1; i <= n; i++)
                R[j][i] = R[j][i] + dt * (epsilon + M1 * R[j][i] / (E[j][i] + M2)) * (-R[j][i] - kk * E[j][i] * (E[j][i] - b - 1));
        double **tmp = E; E = E_prev; E_prev = tmp;
    }
    double el = getTime() - t0;
    double mx = -1, l2 = 0;
    for (int j = 1; j <= m; j++)
        for (int i = 1; i <= n; i++) {
            l2 += E_prev[j][i] * E_prev[j][i];
            if (E_prev[j][i] > mx) mx = E_prev[j][i];
        }
    l2 = sqrt(l2 / ((double)m * n));
    cout << "Number of Iterations        : " << niter << endl;
    cout << "Elapsed Time (sec)          : " << el << endl;
    cout << "Sustained Gflops Rate       : " << niter * 1e-9 * n * (double)n * 28.0 / el << endl << endl;
    cout << "Max: " << mx << " L2norm : " << l2 << endl;
    return 0;
}
