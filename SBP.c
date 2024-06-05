#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <time.h>
#define NUM_WAITING_CHAIRS 5

// Define shared data structure
typedef struct {
    int num_waiting_customers;
    int num_barber_waiting;
    sem_t customer;
    sem_t barber;
    sem_t mutex;
} BarberShop;

void barber(BarberShop *shop) {
    while (1) {
        sem_wait(&shop->customer); // Wait for customer to arrive
        sem_wait(&shop->mutex); // Acquire mutex to update shared data
        shop->num_waiting_customers--; // Decrement number of waiting customers
        sem_post(&shop->mutex); // Release mutex
        sem_post(&shop->barber); // Barber is ready to serve
        printf("Barber is cutting hair\n");
        int rnd = 3 + rand() % 3;
        sleep(rnd);
    }
}

void customer(BarberShop *shop) {
    srand(getpid());
    int rnd = 1 + rand() % 3;
    sleep(rnd);
    sem_wait(&shop->mutex); // Acquire mutex to update shared data
    if (shop->num_waiting_customers < NUM_WAITING_CHAIRS) {
        printf("Customer %d is waiting\n", getpid());
        shop->num_waiting_customers++; // Increment number of waiting customers
        sem_post(&shop->mutex); // Release mutex
        sem_post(&shop->customer); // Notify barber
        sem_wait(&shop->barber); // Wait for barber to be ready
        printf("Customer %d is having a haircut\n", getpid());
    } else {
        printf("Customer %d left because the waiting area is full\n", getpid());
        sem_post(&shop->mutex); // Release mutex
    }
}

int main() {
    int shmid;
    key_t key = IPC_PRIVATE; // Generate unique key for shared memory

    // Create shared memory segment
    if ((shmid = shmget(key, sizeof(BarberShop), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
        perror("shmget");
        exit(1);
    }

    // Attach shared memory segment
    BarberShop *shop = (BarberShop *)shmat(shmid, NULL, 0);
    if (shop == (BarberShop *)(-1)) {
        perror("shmat");
        exit(1);
    }

    // Initialize semaphores
    sem_init(&shop->customer, 1, 0);
    sem_init(&shop->barber, 1, 0);
    sem_init(&shop->mutex, 1, 1);

    // Initialize shared data
    shop->num_waiting_customers = 0;
    shop->num_barber_waiting = 0;

    // Create barber process
    pid_t barber_pid = fork();
    if (barber_pid == 0) {
        barber(shop);
    }
    else
    {
            fork();
            fork();
            fork();
            customer(shop);
    }
    if(barber_pid !=0)
    {
         // Detach and remove shared memory
        shmdt(shop);
        shmctl(shmid, IPC_RMID, NULL);

        // Destroy semaphores
        sem_destroy(&shop->customer);
        sem_destroy(&shop->barber);
        sem_destroy(&shop->mutex);
    }
    return 0;
}
