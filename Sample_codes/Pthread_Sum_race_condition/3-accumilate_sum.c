# include <stdio.h>
# include <pthread.h>
# include <unistd.h>


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


pthread_mutex_t mutex;
void * calculate(void *t_id);
int compute_next_value(int values[], int idx);

int thread_id[8] = {0, 1, 2, 3, 4, 5, 6, 7};

int tasks = NUMBEROFITEMS/NUMBEROFTHREADS;
int sum[NUMBEROFTHREADS] = {0, 0, 0, 0, 0, 0, 0, 0};


int main() {

    pthread_t thread_t[NUMBEROFTHREADS];


    for (int i = 0; i < NUMBEROFTHREADS; i++) {
        pthread_create(&thread_t[i], NULL, calculate, (void *)&thread_id[i]);
    }
    for (int i; i < NUMBEROFTHREADS; i++) {
        pthread_join(thread_t[i], NULL);
    }

    // add the accumilated sum and print it
    int accumilated_sum = 0;
    for (int k; k < NUMBEROFTHREADS; k++) {
    
        accumilated_sum += sum[k];
    }
    // accumilate sum from each threads to avoid locking and to make our 
    // threads run faster without waiting 
    printf("Accumilate sum : %d\n", accumilated_sum);
        return 0;
 }

void * calculate(void *t_id) {
    int result = 0;
    int start =  *(int *)t_id * tasks;
    for (int j = start; j < start + tasks; j++) {
        result += compute_next_value(values, j);
    }
    // accumelate the threads result inside its respective sum
    sum[*(int *)t_id] = result;
    return NULL;
}

int compute_next_value(int values[], int idx){
    usleep(54000);
    return values[idx];
}