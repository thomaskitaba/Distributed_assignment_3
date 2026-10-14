#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>  // For usleep()

#define NUM_THREADS 4
#define N 24

int sum = 0;
int array[N];

void* compute_partial_sum(void* thread_id) {
    long tid = (long)thread_id;
    int items_per_task = N / NUM_THREADS;
    int start = tid * items_per_task;
    int my_x;
    
    printf("Thread %ld: calculating from index %d to %d\n", 
           tid, start, start + items_per_task - 1);
    
    for (int i = start; i < start + items_per_task; i++) {
        my_x = array[i];
        
        // RACE CONDITION: Add delay INSIDE the critical section
        // This makes it more likely that threads will interleave
        
        // Step 1: Read current sum (LOAD)
        int current_sum = sum;
        
        // Step 2: DELAY - thread can be preempted here!
        usleep(10000);  // Sleep for 10 milliseconds
        
        // Step 3: Add value (ADD)
        current_sum += my_x;
        
        // Step 4: DELAY - another preemption point!
        usleep(5000);   // Sleep for 5 milliseconds
        
        // Step 5: Write back (STORE)
        sum = current_sum;
        
        printf("Thread %ld: added %d, sum = %d\n", tid, my_x, sum);
    }
    
    return NULL;
}

void initialize_array() {
    for (int i = 0; i < N; i++) {
        array[i] = i + 1;
    }
}

int main() {
    pthread_t threads[NUM_THREADS];
    int expected_sum = 0;
    
    initialize_array();
    
    for (int i = 0; i < N; i++) {
        expected_sum += array[i];
    }
    
    printf("Expected sum: %d\n", expected_sum);
    printf("Starting parallel computation with delays...%d\n", NUM_THREADS);
    
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, compute_partial_sum, (void*)i);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\nFinal sum: %d (Expected: %d)\n", sum, expected_sum);
    printf("Race condition DETECTED! Sum is incorrect!\n");
    
    return 0;
}

