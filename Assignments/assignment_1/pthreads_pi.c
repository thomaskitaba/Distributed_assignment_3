/********************************************************************

                  Example 3  (pthreads_pi.c)

Objective                  : To calculate the value of pi using the given
                             number of intervals.
                             Demonstrates the usage of:
                             pthread_create()
                             pthread_join()
                             pthread_mutex_t
                             pthread_mutex_lock
                             pthread_mutex_unlock()

Input                      : The number of intervals.

Output                     : Value of Pi using the given intervals.

 ********************************************************************/ 

#include <stdio.h>
#include <pthread.h>

double          intervalWidth, intervalMidPoint, area = 0.0;

int             numberOfIntervals;


 /* Create a MutEx for area. */ 
pthread_mutex_t area_mutex = PTHREAD_MUTEX_INITIALIZER;


 /* Number of Intervals. */ 
int             interval, iCount;

double          myArea = 0.0, result;


void 
myPartOfCalc(int myID)
{
	
	int             myInterval;
	
		double          myIntervalMidPoint;
	
		myArea = 0.0;
	
		
		for (myInterval = myID + 1; myInterval <= numberOfIntervals; myInterval += numberOfIntervals) {
		
			myIntervalMidPoint = ((double) myInterval - 0.5) * intervalWidth;
		
			myArea += (4.0 / (1.0 + myIntervalMidPoint * myIntervalMidPoint));
		
	} 
		result = myArea * intervalWidth;
	
		printf("\n %d: MyArea: %f.", myID, myArea);
	
		
		 /* Lock the mutex controlling the access to area. */ 
		
		pthread_mutex_lock(&area_mutex);
	
		area += result;
	
		pthread_mutex_unlock(&area_mutex);
	
} 

main(int argc, char *argv[])
{
	
		
		 /* Declare a pointer to pthread to create dynamically. */ 
		pthread_t * threads;
	
		
		
		 /* Create a MutEx for area. */ 
		
		pthread_mutex_t area_mutex = PTHREAD_MUTEX_INITIALIZER;
	
		
		 /* Check for the required paramaeters. */ 
		
		if (argc < 2) {
		
			printf(" Usage: pi n.\n n is number of intervals.\n");
		
			exit(-1);
		
	} 
		
		numberOfIntervals = abs(atoi(argv[1]));
	
		if (numberOfIntervals == 0)
		
			numberOfIntervals = 50;
	
		printf("\n No. Of Intervals: %d", numberOfIntervals);
	
		
		 /* Create the threads required. */ 
		
		threads = (pthread_t *) malloc(sizeof(pthread_t) * numberOfIntervals);
	
		
		 /* Calculate Interval Width. */ 
		intervalWidth = 1.0 / (double) numberOfIntervals;
	
		
		 /* Now Compute Area. */ 
		
		 /* We will modify to use the pthreads. */ 
		
		for (iCount = 0; iCount < numberOfIntervals; iCount++)
		
			pthread_create(&threads[iCount], NULL, (void *(*) (void *)) myPartOfCalc, (void *) iCount);
	
		
		
		for (iCount = 0; iCount < numberOfIntervals; iCount++)
		
			pthread_join(threads[iCount], NULL);
	
		
		
		 /* Serial Code 
		for (interval = 0; interval < numberOfIntervals; interval++) {
		
			intervalMidPoint = (interval + 0.5) * intervalWidth;
		
			area += 4.0 / (1.0 + intervalMidPoint * intervalMidPoint);
		
	} 
		
		area *= intervalWidth;
	
		*/
		
		 /* Print the results. */ 
		
		printf("\n The Computed Value of PI %.15f\n", area);
	
		
		return 0;
	
} 
