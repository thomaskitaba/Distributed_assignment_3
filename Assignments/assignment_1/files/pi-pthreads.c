// pi-pthreads.c
// Pthreads implementation of the parallel pi calculation.
// Based on the pi-serial.c template provided in the assignment.
//
// To compile:
//       gcc -o pi-pthreads -O3 pi-pthreads.c -lpthread
//   or on Lufer with the Intel Compiler:
//       icc -o pi-pthreads -O3 pi-pthreads.c -lpthread
//
// To run:
//       ./pi-pthreads ITERS NumThreads > pthreads.out

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

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

// Global variables for the parallel computation.
// Each thread accumulates into its own slot of partialSums, so no
// mutex/lock is required: distinct threads write to distinct memory
// locations and the main thread combines the results after the join.
double sum = 0.0;
long iterationsPerThread;

long   totalIterations;
int    totalThreads;
double *partialSums;

// Method declarations
void* calculatePartialSum (void* threadNumber);
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

// Thread function: each thread sums a strided slice of the series
// (thread id, id+numThreads, id+2*numThreads, ...) and stores its
// partial result in partialSums[id].
void* calculatePartialSum(void* threadNumber)
{
  long id = (long) threadNumber;
  double localSum = 0.0;
  double factor;
  long k;

  for (k = id; k < totalIterations; k += totalThreads)
    {
      // (-1)^k without branching on every iteration's sign flip:
      // even k -> +1, odd k -> -1
      factor = (k % 2 == 0) ? 1.0 : -1.0;
      localSum += factor / (2.0 * k + 1.0);
    }

  partialSums[id] = localSum;

  pthread_exit(NULL);
}

// Parallel computation of PI using Pthreads
double parallelCompute(long iterations, int numberOfThreads)
{
  totalIterations = iterations;
  totalThreads = numberOfThreads;

  pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t) * numberOfThreads);
  partialSums = (double *) malloc(sizeof(double) * numberOfThreads);

  long t;

  // Create the threads; each one runs calculatePartialSum
  for (t = 0; t < numberOfThreads; t++)
    {
      pthread_create(&threads[t], NULL, calculatePartialSum, (void *) t);
    }

  // Wait for all threads to finish
  for (t = 0; t < numberOfThreads; t++)
    {
      pthread_join(threads[t], NULL);
    }

  // Combine partial results (no lock needed here: this is now
  // single-threaded, all worker threads have already joined)
  double totalSum = 0.0;
  for (t = 0; t < numberOfThreads; t++)
    {
      totalSum += partialSums[t];
    }

  free(threads);
  free(partialSums);

  return 4.0 * totalSum;
}
