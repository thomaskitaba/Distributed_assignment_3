#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


#define NUMBEROFITEMS 24
#define NUMBEROFTHREADS 8

int values[] = {
    1, 4, 3,
    9, 2, 8,
    5, 1, 1,
    6, 2, 7,
    2, 5, 0,
    4, 1, 8,
    6, 5, 1,
    2, 3, 9
};


pthread_barrier_t barrier;
void * calculate(void *t_id);
int compute_next_value(int values[], int idx);
int tasks = NUMBEROFITEMS/NUMBEROFTHREADS;
int thread_id[8] = {0, 1, 2, 3, 4, 5, 6, 7};
int sum = 0;
int static local_sum[8];



int main(){
    
    // pthread_barrier_init(&barrier, NULL, NUMBEROFTHREADS);
    pthread_barrier_init(&barrier, NULL, NUMBEROFTHREADS);
    pthread_t thread_t[8];
    for (int i = 0; i < sizeof(thread_id)/sizeof(thread_id[0]); i++) {
        pthread_create(&thread_t[i], NULL, calculate, (void *)&thread_id[i]);
    }

    for (int i = 0; i < sizeof(thread_id)/sizeof(thread_id[0]); i++){
        pthread_join(thread_t[i], NULL);
    }
    
    
        for (int k = 0; k < 8; k++){
            sum += local_sum[k];
        }

    pthread_barrier_destroy(&barrier);
    printf("Expected Global sum: 95\n");
    printf("total sum using barrier: %d\n", sum);
}

void * calculate(void *t_id){

    int id = *(int *)t_id;
    int start = id * tasks;
    for (int j = start; j < start + tasks; j++){
        local_sum[id] += compute_next_value(values, j);
    }

    pthread_barrier_wait(&barrier);

    // if (id == 1) {
    //     for (int k = 0; k < 8; k++){
    //         sum += local_sum[k];
    //     }
    // }

    // printf("Start: %d\n", start);
    return NULL;
}

int compute_next_value(int values[], int idx){
    usleep(54000);
    return values[idx];
}
