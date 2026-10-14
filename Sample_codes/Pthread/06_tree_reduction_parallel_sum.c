#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

#define NUM_THREADS 8
#define N 24

pthread_barrier_t barrier;
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
    
    pthread_barrier_wait(&barrier);
    
    int step = 1;
    int active_threads = NUM_THREADS;
    
    while (step < NUM_THREADS) {
        if (tid % (step * 2) == 0 && tid + step < NUM_THREADS) {
            partial_sums[tid] += partial_sums[tid + step];
            printf("Thread %ld: added partial_sums[%ld], new value = %d\n", 
                   tid, tid + step, partial_sums[tid]);
        }
        
        pthread_barrier_wait(&barrier);
        step *= 2;
    }
    
    if (tid == 0) {
        printf("\nFinal sum (tree reduction): %d\n", partial_sums[0]);
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
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    
    for (int i = 0; i < N; i++) {
        expected_sum += array[i];
    }
    
    printf("Expected sum: %d\n", expected_sum);
    printf("Tree reduction with %d threads\n", NUM_THREADS);
    printf("Number of reduction steps: %d (log2 %d)\n\n", 
           (int)log2(NUM_THREADS), NUM_THREADS);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        partial_sums[i] = 0;
    }
    
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, compute_partial_sum, (void*)i);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_barrier_destroy(&barrier);
    
    return 0;
}
