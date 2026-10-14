#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define NUM_THREADS 8
#define N 10000000

pthread_barrier_t barrier;
int* array;
long long partial_sums[NUM_THREADS];

void* compute_with_mutex(void* thread_id) {
    long tid = (long)thread_id;
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    int items_per_task = N / NUM_THREADS;
    int start = tid * items_per_task;
    int my_x;
    long long my_sum = 0;
    
    for (int i = start; i < start + items_per_task; i++) {
        my_sum += array[i];
    }
    
    pthread_mutex_lock(&mutex);
    partial_sums[0] += my_sum;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

void* compute_reduced_lock(void* thread_id) {
    long tid = (long)thread_id;
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    int items_per_task = N / NUM_THREADS;
    int start = tid * items_per_task;
    long long my_sum = 0;
    
    for (int i = start; i < start + items_per_task; i++) {
        my_sum += array[i];
    }
    
    partial_sums[tid] = my_sum;
    
    pthread_mutex_lock(&mutex);
    if (tid == 0) {
        partial_sums[0] = 0;
        for (int i = 0; i < NUM_THREADS; i++) {
            partial_sums[0] += partial_sums[i];
        }
    }
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

void* compute_tree_reduction(void* thread_id) {
    long tid = (long)thread_id;
    int items_per_task = N / NUM_THREADS;
    int start = tid * items_per_task;
    long long my_sum = 0;
    
    for (int i = start; i < start + items_per_task; i++) {
        my_sum += array[i];
    }
    
    partial_sums[tid] = my_sum;
    
    pthread_barrier_wait(&barrier);
    
    int step = 1;
    while (step < NUM_THREADS) {
        if (tid % (step * 2) == 0 && tid + step < NUM_THREADS) {
            partial_sums[tid] += partial_sums[tid + step];
        }
        pthread_barrier_wait(&barrier);
        step *= 2;
    }
    
    return NULL;
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main() {
    array = (int*)malloc(N * sizeof(int));
    if (array == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
    }
    
    for (int i = 0; i < N; i++) {
        array[i] = 1;
    }
    
    pthread_t threads[NUM_THREADS];
    double start_time, end_time;
    long long expected_sum = N;
    
    printf("Performance Comparison - N = %d elements, %d threads\n", N, NUM_THREADS);
    printf("Expected sum: %lld\n\n", expected_sum);
    
    printf("1. Mutex approach:\n");
    partial_sums[0] = 0;
    start_time = get_time();
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, compute_with_mutex, (void*)i);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    end_time = get_time();
    printf("   Result: %lld, Time: %.4f seconds\n", partial_sums[0], end_time - start_time);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        partial_sums[i] = 0;
    }
    
    printf("\n2. Reduced locking approach:\n");
    start_time = get_time();
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, compute_reduced_lock, (void*)i);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    end_time = get_time();
    printf("   Result: %lld, Time: %.4f seconds\n", partial_sums[0], end_time - start_time);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        partial_sums[i] = 0;
    }
    
    printf("\n3. Tree reduction approach:\n");
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    start_time = get_time();
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, compute_tree_reduction, (void*)i);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    end_time = get_time();
    printf("   Result: %lld, Time: %.4f seconds\n", partial_sums[0], end_time - start_time);
    pthread_barrier_destroy(&barrier);
    
    free(array);
    
    return 0;
}
