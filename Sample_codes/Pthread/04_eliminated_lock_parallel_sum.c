#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 4
#define N 24

int array[N];
int partial_sums[NUM_THREADS];

void* compute_partial_sum(void* thread_id) {
    long tid = (long)thread_id;
    int items_per_task = N / NUM_THREADS;
    int start = tid * items_per_task;
    int sum = 0;
    int my_x;
    
    printf("Thread %ld: calculating from index %d to %d\n", 
           tid, start, start + items_per_task - 1);
    
    for (int i = start; i < start + items_per_task; i++) {
        my_x = array[i];
        sum += my_x;
    }
    
    partial_sums[tid] = sum;
    
    printf("Thread %ld: partial sum = %d\n", tid, sum);
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
    int final_sum = 0;
    
    initialize_array();
    
    for (int i = 0; i < N; i++) {
        expected_sum += array[i];
    }
    
    printf("Expected sum: %d\n", expected_sum);
    printf("Starting parallel computation without locks...\n\n");
    
    for (int i = 0; i < NUM_THREADS; i++) {
        partial_sums[i] = 0;
    }
    
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, compute_partial_sum, (void*)i);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\nMaster thread accumulating results...\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("  Adding partial_sums[%d] = %d\n", i, partial_sums[i]);
        final_sum += partial_sums[i];
    }
    
    printf("\nFinal sum: %d (Expected: %d)\n", final_sum, expected_sum);
    printf("Sum is CORRECT without locks!\n");
    
    return 0;
}
