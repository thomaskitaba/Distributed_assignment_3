// #include <stdio.h>
// #include <unistd.h>
// #include <pthread.h>


// void *thread_function(void *num) ;
// int original = 0;

// int main() {
  
//     pthread_t thread_1;
//     pthread_t thread_2;
    
//     int arg = 10;
//     int result_1 = 1, result_2 = 1;

//     result_1 = pthread_create(&thread_1, NULL, thread_function, &arg);
//     if (result_1 == 0) {
//         printf("Thread 1 created successfully\n");
//     }else{
//         printf("Error: creating thread\n");
//     }
    
   
//     result_2 = pthread_create(&thread_2, NULL, thread_function, &arg);
//      if (result_2 == 0) {
//         printf("Thread 1 created successfully\n");
//     }else{
//         printf("Error: creating thread\n");
//     }
     
//     pthread_join(thread_1, NULL);
//     pthread_join(thread_2, NULL);
//     // printf("Original value on local thread_2: %d\n", original);
    
//     return 0;
// }

// // thread function should accept 
// void *thread_function(void *num)
// {
//     // int thread_var = *(int *)num;
//     usleep(50000);
//     original += *(int *)num;
//     usleep(50000);
//     printf("original varables value in thread: %d\n", original);
//     return NULL;
// }

// #include <stdio.h>
// #include <unistd.h>
// #include <pthread.h>


// void *deposit(int *amount, void *num) ;


// int main(void) {
//     printf("hello tom");
    
//     pthread_t  thread_1;  // identifier for the thread
//     pthread_t thread_2;   // identifier for the thread

//     int amount = 0;
//     int num1 = 10, num2 = 20;
    
//     pthread_create(&thread_1, NULL, amount, &num1);
//     pthread_create(&thread_1, NULL, amount, &num1);



//     return 0;
// }

// // thread function should accept 
// void *deposit(int *amount, void *num) 
// {
//     amount = amount + *(int *)num;
//     printf("%d", *amount);
// }


#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void *thread_function(void *num);

int original = 0;   // Shared global variable. Both threads read AND write
                     // this same memory location with no lock protecting
                     // it -- that's what makes this a race condition.

int main() {

    pthread_t thread_1;
    pthread_t thread_2;

    int arg = 10;
    int result_1 = 1, result_2 = 1;

    result_1 = pthread_create(&thread_1, NULL, thread_function, &arg);
    if (result_1 == 0) {
        printf("Thread 1 created successfully\n");
    } else {
        printf("Error: creating thread\n");
    }

    result_2 = pthread_create(&thread_2, NULL, thread_function, &arg);
    if (result_2 == 0) {
        printf("Thread 2 created successfully\n");
    } else {
        printf("Error: creating thread\n");
    }

    pthread_join(thread_1, NULL);   // wait for thread_1 to finish
    pthread_join(thread_2, NULL);   // wait for thread_2 to finish

    printf("Final value of original in main: %d\n", original);
    // EXPECTED (if the two additions never overlapped): 10 + 10 = 20
    // ACTUAL: often 10, because of the race explained below.

    return 0;
}

void *thread_function(void *num)
{
    // ------------------------------------------------------------------
    // original += *(int *)num;   <-- this single line is NOT one atomic
    // operation at the machine level. The CPU actually does THREE
    // separate steps to carry it out:
    //
    //   1. READ  original from memory into a local/register copy
    //   2. ADD   *num to that local copy
    //   3. WRITE the updated local copy back to original in memory
    //
    // Normally these three steps happen so fast that thread_1 and
    // thread_2 don't get scheduled in a way that overlaps them, so you
    // don't SEE the race even though it's still technically there
    // (this is exactly what happened when we ran the previous version
    // 5 times and always got 20 -- the bug was present, just invisible).
    //
    // Below, we manually break the single "+=" line into its three
    // steps and insert a usleep() in the middle. This artificially
    // widens the gap between step 1 (read) and step 3 (write), giving
    // the OS scheduler time to swap in the OTHER thread while this one
    // is "paused" -- which is exactly what makes the race visible.
    // ------------------------------------------------------------------

    int local_copy = original;          // STEP 1: read shared value
    printf("[thread] read original = %d\n", local_copy);

    usleep(100000);                     // STEP 1.5: sleep 100ms -- during
                                         // this pause, the other thread
                                         // gets scheduled and does its
                                         // OWN read/add/write on the
                                         // SAME original variable

    local_copy = local_copy + *(int *)num;   // STEP 2: add to the LOCAL copy
                                              // (not to `original` directly)

    original = local_copy;              // STEP 3: write back to shared value
                                         // -- this can silently OVERWRITE
                                         // whatever the other thread wrote,
                                         // if it already finished during
                                         // our sleep

    printf("[thread] wrote original = %d\n", original);

    return NULL;
}