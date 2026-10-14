// pi-openmp.c
// OpenMP implementation of the parallel pi calculation.
// Based on the pi-serial.c template provided in the assignment.
//
// To compile on Lufer with the Intel Compiler:
//       icc -o pi-openmp -O3 -openmp pi-openmp.c
// With gcc:
//       gcc -o pi-openmp -O3 -fopenmp pi-openmp.c
//
// To run:
//       ./pi-openmp ITERS NumThreads > openmp.out

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>

double getTime()
{
  const double kMicro = 1.0e-6;
  struct timeval TV;

  const int RC = gettimeofday(&TV, NULL);
  if(RC == -1)
    {
      printf("ERROR: Bad call to gettimeofday\n");
      return(-1);
    }
  return( ((double)TV.tv_sec) + kMicro * ((double)TV.tv_usec) );
}

// Global variables
double sum = 0.0;
long iterationsPerThread;

// Method declarations
double sequentialCompute (long iterations);
double parallelCompute(long iterations, int numberOfThreads);
double getDifference(double calculatedPi);

// Main method
int main(int argc, char* argv[])
{
  // Variable declarations
  double sequentialStart, sequentialEnd, sequentialTime;
  double parallelStart, parallelEnd, parallelTime;

  double sequentialPi, parallelPi;
  double sequentialDifference, parallelDifference;
  long iterations; 
  int numberOfThreads;

  // Get number of iterations and number of threads from the command line
  if(argc > 1)
    {
      iterations = strtol(argv[1], NULL, 10);
      numberOfThreads = strtol(argv[2], NULL, 10);
    }
  else
    {
      printf("\nWhen running this program, please include number of iterations and number of threads on command line.\n");
      return 0;
    }

  // Time sequential calculation
  sequentialStart = getTime();
  sequentialPi = sequentialCompute(iterations);
  sequentialEnd = getTime();
  sequentialTime = sequentialEnd - sequentialStart;
  
  // Time parallel calculation
  parallelStart = getTime();
  parallelPi = parallelCompute(iterations, numberOfThreads);
  parallelEnd = getTime();
  parallelTime = parallelEnd - parallelStart;
  
  // How do results compare with PI?
  sequentialDifference = getDifference(sequentialPi);
  parallelDifference = getDifference(parallelPi);
  
  // Print results
  printf("Sequential Calculation: %f\n", sequentialPi);
  printf("This is %f away from the correct value of PI.\n\n", sequentialDifference);
  printf("ParallelCalculation: %f\n", parallelPi);
  printf("This is %f away from the correct value of PI.\n\n", parallelDifference);
  printf("Number of iterations: %ld, Number of Threads: %d\n\n", iterations, numberOfThreads);

  // Calculate the validity of the parallel computation
  double difference = parallelDifference - sequentialDifference;

  if (difference < .01 && difference > -.01)
    printf("Parallel calculation is VALID!\n");
  else
    printf("Parallel calculation is INVALID!\n");

  // Calculate and print speedup results
  double speedup = ((double)sequentialTime)/((double)parallelTime);
  printf("Sequential Time: %f, Parallel Time: %f, Speedup: %f\n", sequentialTime, parallelTime, speedup);

  return 0;
}

// Sequential computation of PI
double sequentialCompute (long iterations)
{
  double factor = 1.0;
  double sum = 0;
  double pi_approximation; 

  long k = 0;
  for (k = 0; k < iterations; k++)
    {
      sum += factor/(2*k+1);
      factor = -factor;
    }

  pi_approximation = 4.0 * sum; 
  return pi_approximation;
}

// Find how close the calculation is to the actual value of PI
double getDifference(double calculatedPi)
{
  return calculatedPi - 3.14159265358979323846;
}

// Parallel computation of PI using OpenMP
double parallelCompute(long iterations, int numberOfThreads)
{
  double localSum = 0.0;
  long k;

  omp_set_num_threads(numberOfThreads);

  // Each thread accumulates into its own private copy of localSum;
  // OpenMP's reduction clause combines them safely at the end of the
  // loop, avoiding any explicit locking.
  #pragma omp parallel for reduction(+:localSum) private(k) schedule(static)
  for (k = 0; k < iterations; k++)
    {
      double factor = (k % 2 == 0) ? 1.0 : -1.0;
      localSum += factor / (2.0 * k + 1.0);
    }

  return 4.0 * localSum;
}