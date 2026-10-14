# include <stdio.h>
# include <pthread.h>
# include <unistd.h>


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
int sum = 0;
int static thread_id[8] = {0, 1, 2, 3, 4, 5, 6, 7};
int compute_next_value(int values[], int idx) ;


void *calculate(void *num);


int main(){

    pthread_mutex_init(&mutex, NULL);
    pthread_t thread_t[8];
    printf("%ld\n", sizeof(thread_id));
    printf("%ld\n", sizeof(thread_id)[0]);
    
    for (int t_id = 0; t_id < sizeof(thread_id)/sizeof(thread_id)[0]; t_id++) {
        pthread_create(&thread_t[t_id], NULL, calculate, (void *)&thread_id[t_id]);
        // printf("sent t_id: %d \n", thread_id[t_id]);
    }
     for (int t_id = 0; t_id < sizeof(thread_id)/sizeof(thread_id)[0]; t_id++) {
        pthread_join(thread_t[t_id], NULL);
        // printf("sent t_id: %d \n", thread_id[t_id]);
    }
    pthread_mutex_destroy(&mutex);
    printf("SUM after ALL threads .. : %d", sum);
    return 0;
}


int compute_next_value(int values[], int idx){
    usleep(54000);
    return values[idx];
}


void *calculate(void *thread_id){
    int n = 24, t = 8;    // n = number of items      t = number of threads
    int items_per_thread = n/t;  //equal to the number of threads
    // printf("accepted t_id: %d \n", *(int *)thread_id);
    int start = *(int *)thread_id * items_per_thread;   
    // printf("start possition for each thread: %d\n", start);

    for (int i = start; i < start + items_per_thread; i++) {
        pthread_mutex_lock(&mutex);
        sum += compute_next_value(values, i);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;

}



// #include <stdio.h>
// #include <pthread.h>
// #include <unistd.h>

// #define NUM_VALUES 24
// #define NUM_THREADS 8

// int values[] = {
//     1, 4, 3,
//     9, 2, 8,
//     5, 1, 1,
//     6, 2, 7,
//     2, 5, 0,
//     4, 1, 8,
//     6, 5, 1,
//     2, 3, 9
// };

// int sum = 0;

// static int thread_id[NUM_THREADS] = {
//     0, 1, 2, 3,
//     4, 5, 6, 7
// };

// pthread_mutex_t sum_mutex = PTHREAD_MUTEX_INITIALIZER;


// // Function prototypes
// int compute_next_value(int values[], int idx);
// void *calculate(void *arg);


// int main()
// {
//     pthread_t threads[NUM_THREADS];

//     // Display the size of the thread_id array
//     printf("Size of thread_id array: %zu bytes\n", sizeof(thread_id));
//     printf("Size of one thread_id element: %zu bytes\n", sizeof(thread_id[0]));

//     // Create threads
//     for (int i = 0; i < NUM_THREADS; i++)
//     {
//         pthread_create(
//             &threads[i],
//             NULL,
//             calculate,
//             &thread_id[i]
//         );
//     }

//     // Wait for all threads to finish
//     for (int i = 0; i < NUM_THREADS; i++)
//     {
//         pthread_join(threads[i], NULL);
//     }

//     printf("SUM after all threads: %d\n", sum);

//     pthread_mutex_destroy(&sum_mutex);

//     return 0;
// }


// // Returns the value at the specified index
// int compute_next_value(int values[], int idx)
// {
//     return values[idx];
// }


// // Thread function
// void *calculate(void *arg)
// {
//     int id = *(int *)arg;

//     // Divide the work equally among the threads
//     int items_per_thread = NUM_VALUES / NUM_THREADS;

//     // Determine where this thread should start
//     int start = id * items_per_thread;

//     // Calculate the sum of this thread's assigned values
//     for (int i = start;
//          i < start + items_per_thread;
//          i++)
//     {
//         int value = compute_next_value(values, i);

//         // Protect the shared variable 'sum'
//         pthread_mutex_lock(&sum_mutex);
//         sum += value;
//         pthread_mutex_unlock(&sum_mutex);
//     }

//     return NULL;
// }
