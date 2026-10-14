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
int sum = 0;
int total_sum = 0;

int main(){

    pthread_mutex_init(&mutex, NULL);

    // create array of theread
    pthread_t thread_t[NUMBEROFTHREADS];

    // create threads
    for (int i = 0; i < sizeof(thread_id)/sizeof(thread_id[0]); i++) {
        pthread_create(&thread_t[i], NULL, calculate, (void *)&thread_id[i]);
    }

    for (int i = 0; i < sizeof(thread_id)/sizeof(thread_id[0]); i++) {
        pthread_join(thread_t[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    printf("================================");
    printf("Expected Sum:  %d\n", 95);
    printf("redcuced locke:  %d", total_sum);
    return 0;
}

void * calculate(void *t_id){
    // printf("Hello there?\n");
    
    int start = *(int *)t_id * tasks;
    printf("start index:  %d  - end index: %d\n ", start, start + tasks - 1);
    int temp_j = start;
    
    for (int j = start; j < start + tasks; j++) {

        sum += compute_next_value(values, j);
        temp_j = j;
        // printf("local sum: %d\n", sum);
    }


    pthread_mutex_lock(&mutex);
    total_sum += sum;
    pthread_mutex_unlock(&mutex);
    return NULL;
}


int compute_next_value(int values[], int idx) {
    usleep(54000);
    return values[idx];
}


// When instructions execute that fast, Thread 0 can often start, compute its value, and finish printing its printf before the operating system even has a chance to schedule Thread 1. Because they finish sequentially by pure luck, it looks like there is no race condition or interleaving, even without a mutex.
// When you introduce usleep, you are explicitly telling the operating system: "Pause this thread right now and give the CPU to someone else."
// This creates a massive time window. Instead of the thread racing through its task instantly, it pauses mid-flight. This forces the OS scheduler to jump in, switch context to other