#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>
#define SHM_SIZE 1024

// Define the shared data structure
typedef struct {
    int data;
    sem_t mutex;
    sem_t wrt;
    int read_count;
} SharedData;

void writer(SharedData *shared_data) {
    while (1) {
        sem_wait(&shared_data->wrt); // Wait for exclusive access
        shared_data->data++; // Write to shared data
        printf("Writer wrote: %d\n", shared_data->data);
        sem_post(&shared_data->wrt); // Release exclusive access
        sleep(1); // Simulate some work being done
    }
}

void reader(SharedData *shared_data) {
    srand(getpid());
    while (1) {
        sem_wait(&shared_data->mutex); // Acquire mutex to update read_count
        shared_data->read_count++;
        if (shared_data->read_count == 1) {
            sem_wait(&shared_data->wrt); // First reader blocks writer
        }
        sem_post(&shared_data->mutex); // Release mutex

        // Read data
        printf("Reader %d read: %d, Number of readers :%d\n", getpid(), shared_data->data,shared_data->read_count);

        sem_wait(&shared_data->mutex); // Acquire mutex to update read_count
        shared_data->read_count--;
        if (shared_data->read_count == 0) {
            printf("Writer lock released...\n");
            sem_post(&shared_data->wrt); // Last reader unblocks writer
        }
        sem_post(&shared_data->mutex); // Release mutex
        int rnd = 1 + rand() % 4;
        sleep(rnd); // Simulate some work being done
    }
}

int main() {
    int shmid;
    key_t key = IPC_PRIVATE; // Generate unique key for shared memory

    // Create shared memory segment
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
        perror("shmget");
        exit(1);
    }

    // Attach shared memory segment
    SharedData *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (SharedData *)(-1)) {
        perror("shmat");
        exit(1);
    }

    // Initialize semaphores
    sem_init(&shared_data->mutex, 1, 1); // Binary semaphore for mutual exclusion
    sem_init(&shared_data->wrt, 1, 1);   // Binary semaphore for writer

    // Initialize shared data
    shared_data->data = 0;
    shared_data->read_count = 0;

    // Create writer process
    pid_t writer_pid = fork();
    if (writer_pid == 0) {
        writer(shared_data);
    }
    else
    {
        // Create reader processes
        fork();
        fork();
        reader(shared_data);
    }

    // Detach and remove shared memory
    shmdt(shared_data);
    shmctl(shmid, IPC_RMID, NULL);

    // Destroy semaphores
    sem_destroy(&shared_data->mutex);
    sem_destroy(&shared_data->wrt);

    return 0;
}
