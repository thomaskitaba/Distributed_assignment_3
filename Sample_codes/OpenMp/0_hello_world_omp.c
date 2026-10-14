# include <stdio.h>
# include <omp.h>
# include <unistd.h>



int main(int argc, char* argv[]){

    
    int balance = 0;
    // #pragma "compiler, pay special attention to this instruction"
    // #pragma omp "this is an openmp instruction"
    // #pragma omp parallel   "create multiple threads and let them
    // execute the code in side the block"
    #pragma omp parallel 
    {
        usleep(54000);
        #pragma omp critical
        {
            balance += 100;
        }
        
        int threads = omp_get_num_threads();
        int current_thread = omp_get_thread_num();
        printf("threads: %d-- thread_id: %d -- balance: %d\n", threads, current_thread, balance);

    }

    int counter = 0;
    #pragma omp parallel
    {   
        #pragma omp atomic
        counter++;
        #pragma omp critical
        {    
            printf("counter: %d\n", counter);
        }
    }
    return 0;

}
































// #include <stdio.h>
// #include <stdlib.h>
// #include <omp.h>
// #include <unistd.h>

// int main(int argc, char* argv[]) {

//     // since the variables are declared outside 
//     // this will create race condition 

//     int threads, current_thread;   // comment this line to create the race condtion
//     #pragma omp parallel 
//     {
//         usleep(100000);
//         // int threads, current_thread;     //uncomment this remove race condition
//         threads = omp_get_num_threads();
//         current_thread = omp_get_thread_num();

//         printf("Hello world printed by thread %d__ working threads: %d\n", current_thread, threads);
//     }
//     return 0;
// }