// Instructions on how to use this code:
// Create two versions of the code, one for OpenMP and one for Pthreads.
// For each, provide an implementation of the parallel pi calculation.
// Example file names for these programs are pi-openmp.c and pi-pthreads.c. 


// To compile OpenMP on Lufer with Intel Compiler, use the following: 
//       icc -o pi-openmp -O3 -openmp pi-openmp.c
// To compile for Pthreads, use the following: 
//       icc -o pi-pthreads -O3 pi-pthreads.c -lpthread 
// Run OpenMP version with
//       ./pi-openmp ITERS NumThreads > openmp.out
// And Pthreads version with
//       ./pi-pthreads ITERS NumThreads > pthreads.out 

// Modified by : Didem Unat
// Original Author: Mary Hall

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

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

// TODO: Uncomment one of these include files for each version
//#include "pthread.h"
//#include "omp.h"

// TODO: Declare any global variables or types here
double sum = 0.0;
long iterationsPerThread;

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

// TODO: You need to implement this function.
// Parallel computation of PI
double parallelCompute(long iterations, int numberOfThreads)
{
    // TODO: This is the code you need to write for both OpenMP and Pthreads.
    // TODO: It should initialize the number of threads.  For pthreads, it
    // TODO: needs to create the threads.  Then it performs the pi computation 
    // TODO: in parallel.  Each thread performs the sum computation to arrive
    // TODO: at global sum.  Finally, it returns the result. 

  return 0.0; 
}


