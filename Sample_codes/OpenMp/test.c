# include <stdio.h>
# include <omp.h>
# include <unistd.h>



int main (int argc, char *argv){

    int sum = 0;
    int threads, thread_id;

    #pragma omp parallel
    {
        #pragma omp atomic
        sum += 100;
        # pragma opm critical
        {
            
        
            threads = omp_get_num_threads();
            thread_id = omp_get_thread_num();
            printf("Sum: --%d-- threads: %d__ thread_id: %d\n", sum, threads, thread_id);
        }
    }


    return 0;
}