

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>


int balance = 0;
pthread_mutex_t mutex;

int read_balance();
void wirte_balance(int new_balance); 
void *deposit(void *amount);


int main(void) {

    pthread_t thread_1;
    pthread_t thread_2;
    pthread_mutex_init(&mutex, NULL);

    int deposit_1 = 200;
    int deposit_2 = 300;

    pthread_create(&thread_1, NULL, deposit, (void *) &deposit_1);
    pthread_create(&thread_2, NULL, deposit, (void *) &deposit_2);
    int before_deposting = read_balance();
    printf("Before deposing: %d\n", before_deposting);

    pthread_join(thread_1, NULL);
    pthread_join(thread_2, NULL);

    pthread_mutex_destroy(&mutex);
    int after_deposting = read_balance();
    printf("After deposing: %d\n", after_deposting);

}


int read_balance(){
    usleep(25000);
    return balance;
}

void wirte_balance(int new_balance) {
    usleep(25000);
    balance = new_balance; 
}

void *deposit(void *amount) {

    pthread_mutex_lock(&mutex); // Comment this to get race condition
    int account_blance = read_balance();
    
    account_blance += *(int *)amount; // lock the critical section
    
    wirte_balance(account_blance);
    pthread_mutex_unlock(&mutex); // Comment this to get race condition

    return NULL;
}