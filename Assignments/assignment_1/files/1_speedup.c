#include <stdio.h>
#include <math.h>

int main() {
    int n_values[] = {10, 20, 40, 80, 160, 320};
    int p_values[] = {1, 2, 4, 8, 16, 32, 64, 128};
    int nCount = 6, pCount = 8;

    printf("%-6s %-6s %-14s %-14s %-10s %-10s\n",
           "n", "p", "Tserial", "Tparallel", "Speedup", "Efficiency");

    for (int i = 0; i < nCount; i++) {
        double n = n_values[i];
        double Tserial = n * n;
        for (int j = 0; j < pCount; j++) {
            double p = p_values[j];
            double Tparallel = (n * n) / p + log2(p);
            double speedup = Tserial / Tparallel;
            double efficiency = speedup / p;
            printf("%-6.0f %-6.0f %-14.4f %-14.4f %-10.4f %-10.4f\n",
                   n, p, Tserial, Tparallel, speedup, efficiency);
        }
        printf("\n");
    }
    return 0;
}
