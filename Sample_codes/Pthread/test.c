# include <stdio.h>
# include <unistd.h>
# include <pthread.h>
# include <omp.h>

pthread_mutex_t mutex;
int sum = 0;
int omp_sum = 0;
void *calculate_sum(void *val){

    pthread_mutex_lock(&mutex);
    sum = sum + *((int *)val);
    pthread_mutex_unlock(&mutex);
    return NULL;
}


int main(){

    pthread_t thread_1, thread_2;
    int num_1 = 100;
    int num_2 = 200;
    
        pthread_mutex_init(&mutex, NULL);

        printf("Sum before: %d\n", 0);

        pthread_create(&thread_1, NULL, calculate_sum, (void *) &num_1);
        pthread_create(&thread_2, NULL, calculate_sum, (void *) &num_2);

        pthread_join(thread_1, NULL);
        pthread_join(thread_2, NULL);

        pthread_mutex_destroy(&mutex);
        printf("Sum After: %d\n", sum);


    printf("***********************\n");
    int threads, thread_id;

    #pragma omp parallel
    {
        threads = omp_get_num_threads();
        thread_id = omp_get_thread_num();
        printf("threads: %d ___ Thread_id: %d\n", threads, thread_id);
    }


    return 0;
}