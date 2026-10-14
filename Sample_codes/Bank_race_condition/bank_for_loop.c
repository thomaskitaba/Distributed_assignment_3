

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


int balance = 0;
int read_balance();
int wirte_balance(int new_balance);
void *deposit(void * amount);
pthread_mutex_t mutex;

int main() {
    pthread_t thread_t[5];
    pthread_mutex_init(&mutex, NULL);
    int deposit_amount[5] = {100, 200, 300, 400, 500};

    // int before_deposting = read_balance();

    printf("Before Deposit: %d\n", read_balance());

    for (int i; i < 5; i++){
        pthread_create(&thread_t[i], NULL, deposit, (void *) &deposit_amount[i]);
        printf("thread %d created successfully\n", i);

    }

    for (int i; i < 5; i++) {

        pthread_join(thread_t[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    printf("After Deposit: %d\n", read_balance());
    return 0;
}

int read_balance() {
    usleep(54000);
    return balance;
}

int wirte_balance(int new_balance) {
    usleep(54000);
    balance = new_balance; // write the new balance on global balance variable
}

void * deposit(void *amount) {
    
    // add lock here
    pthread_mutex_lock(&mutex);
    int account_blance = read_balance(); //read to local thread memory
    account_blance += *(int *)amount; // local blance
    wirte_balance(account_blance); // write local balance to global balance
    // add unlock here
    pthread_mutex_unlock(&mutex);
    return NULL;
    
}