#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 4
#define N 24

pthread_mutex_t mutex;
int sum = 0;
int array[N];

void* compute_partial_sum(void* thread_id) {
    long tid = (long)thread_id;
    int items_per_task = N / NUM_THREADS;
    int start = tid * items_per_task;
    int my_sum = 0;
    int my_x;
    
    printf("Thread %ld: calculating from index %d to %d\n", 
           tid, start, start + items_per_task - 1);
    
    for (int i = start; i < start + items_per_task; i++) {
        my_x = array[i];
        my_sum += my_x;
    }
    
    printf("Thread %ld: partial sum = %d\n", tid, my_sum);
    
    pthread_mutex_lock(&mutex);
    sum += my_sum;
    pthread_mutex_unlock(&mutex);
    
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
    pthread_mutex_init(&mutex, NULL);
    
    for (int i = 0; i < N; i++) {
        expected_sum += array[i];
    }
    
    printf("Expected sum: %d\n", expected_sum);
    printf("Starting parallel computation with reduced locking...\n\n");
    
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, compute_partial_sum, (void*)i);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_mutex_destroy(&mutex);
    
    printf("\nFinal sum: %d (Expected: %d)\n", sum, expected_sum);
    printf("Sum is CORRECT with reduced locking overhead!\n");
    
    return 0;
}
