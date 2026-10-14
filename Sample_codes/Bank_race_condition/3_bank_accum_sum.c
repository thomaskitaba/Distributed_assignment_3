#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int balance = 0;
int read_balance();
int wirte_balance(int new_balance);
void *deposite(void *amount);
int accumilated_sum[2] = {0, 0};
pthread_mutex_t mutex;

typedef struct data{
    int thread_id;
    int deposit;
    char *thread_name;
} thread_data;


int main(){
    
    pthread_mutex_init(&mutex,);
    pthread_t thread_1, thread_2;
    thread_data data_1 = {0, 200, "thread_1"};
    thread_data data_2 = {1, 300, "thread_2"};


    printf("Before deposite: %d \n", read_balance());

    pthread_create(&thread_1, NULL, deposite, (void *)&data_1);
    pthread_create(&thread_2, NULL, deposite, (void *)&data_2);

    pthread_join(thread_1, NULL);
    pthread_join(thread_2, NULL);

    
    printf("After deposite: %i \n", read_balance());

    pthread_mutex_destroy(&mutex);

    // int total_sum = 0;
    // for (int i; i < 2; i++){
    //     total_sum += accumilated_sum[i];
    // }
    // printf("Accumilated SUM:  %d\n", total_sum);

    return 0;
}

int read_balance(){
    usleep(54000);
    return balance;
}

int wirte_balance(int new_balance){
    usleep(54000);
    balance = new_balance;
    return 0;
}

void *deposite(void *data){
    thread_data *t_data = (thread_data *)data;
    // printf("%d    -- %d\n", t_data->thread_id, t_data->deposit);
    // we can use the . operator to access members of the thread_data struct
    // thread_data t_data_2 = *(thread_data *)data;
    // printf("%d    -- %d\n", t_data_2.thread_id, t_data_2.deposit);

    pthread_mutex_lock(&mutex);
    if (t_data->thread_id == 0) {
        int new_balance = read_balance();
        new_balance += t_data->deposit;
        wirte_balance(new_balance);
        // accumilated_sum[t_data->thread_id] = new_balance;
    }
    if (t_data->thread_id == 1) {
        int new_balance = read_balance();
        new_balance += t_data->deposit;
        wirte_balance(new_balance);
        // accumilated_sum[t_data->thread_id] = new_balance;
    }
    pthread_mutex_unlock(&mutex);
}