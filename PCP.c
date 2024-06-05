#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#define N 5
#define SHM_SIZE 1024

// Shared memory structure
typedef struct {
    int buffer[N];
    int in;
    int out;
    sem_t mutex;
    sem_t empty;
    sem_t full;
} shared_memory;

// Shared memory pointer
shared_memory *shm;

// Function for producer process
void producer() {
    int item;
    for (int i = 0; i < 10; i++) {
        item = rand() % 100; // Produce an item

        // Wait if buffer is full
        sem_wait(&shm->empty);
        // Enter critical section
        sem_wait(&shm->mutex);

        // Add item to buffer
        shm->buffer[shm->in] = item;
        printf("Producer produced %d\n", item);
        shm->in = (shm->in + 1) % N;

        // Exit critical section
        sem_post(&shm->mutex);
        // Signal that buffer is not empty
        sem_post(&shm->full);

        sleep(1);
        if(shm->in == N-1)
            sleep(15);
    }
}

// Function for consumer process
void consumer() {
    int item;
    srand(getpid());
    sleep(3);
    for (int i = 0; i < 10; i++) {
        // Wait if buffer is empty
        sem_wait(&shm->full);
        // Enter critical section
        sem_wait(&shm->mutex);

        // Remove item from buffer
        item = shm->buffer[shm->out];
        printf("Consumer %d consumed %d\n", getpid(), item);
        shm->out = (shm->out + 1) % N;

        // Exit critical section
        sem_post(&shm->mutex);
        // Signal that buffer is not full
        sem_post(&shm->empty);
        int rnd = 2 + rand() % 5;
        sleep(rnd);
    }
}

int main() {
    key_t key = IPC_PRIVATE;
    int shmid;

    // Create shared memory segment
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | S_IRUSR | S_IWUSR)) == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment
    shm = (shared_memory *)shmat(shmid, NULL, 0);
    if (shm == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Initialize indices
    shm->in = 0;
    shm->out = 0;

    // Initialize semaphores
    sem_init(&shm->mutex, 1, 1);
    sem_init(&shm->empty, 1, N);
    sem_init(&shm->full, 1, 0);

    // Create producer process
    pid_t pid = fork();
    if (pid == 0) {
        fork();
        consumer();
        exit(EXIT_SUCCESS);
    } else {
        producer();
        wait(NULL);
        sem_destroy(&shm->mutex);
        sem_destroy(&shm->empty);
        sem_destroy(&shm->full);

        if (shmdt(shm) == -1) {
            perror("shmdt");
            exit(EXIT_FAILURE);
        }
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    return 0;
}
