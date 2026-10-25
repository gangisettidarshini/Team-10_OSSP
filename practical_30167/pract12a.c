#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define ITEMS 20

int buffer[BUFFER_SIZE];

int in = 0;
int out = 0;

sem_t empty;
sem_t full;

pthread_mutex_t mutex;

void *producer(void *arg)
{
    int i;

    for (i = 1; i <= ITEMS; i++)
    {
        // Wait for an empty position
        sem_wait(&empty);

        // Lock buffer
        pthread_mutex_lock(&mutex);

        buffer[in] = i;

        printf("Producer produced: %d\n", i);

        in = (in + 1) % BUFFER_SIZE;

        // Unlock buffer
        pthread_mutex_unlock(&mutex);

        // Increase full count
        sem_post(&full);

        usleep(100000);
    }

    return NULL;
}

void *consumer(void *arg)
{
    int i;
    int item;

    for (i = 1; i <= ITEMS; i++)
    {
        // Wait for available item
        sem_wait(&full);

        // Lock buffer
        pthread_mutex_lock(&mutex);

        item = buffer[out];

        printf("Consumer consumed: %d\n", item);

        out = (out + 1) % BUFFER_SIZE;

        // Unlock buffer
        pthread_mutex_unlock(&mutex);

        // Increase empty count
        sem_post(&empty);

        usleep(150000);
    }

    return NULL;
}

int main()
{
    pthread_t producerThread;
    pthread_t consumerThread;

    // Initialize semaphores
    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&full, 0, 0);

    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);

    printf("Producer-Consumer Problem\n");
    printf("==========================\n\n");

    pthread_create(&producerThread,
                   NULL,
                   producer,
                   NULL);

    pthread_create(&consumerThread,
                   NULL,
                   consumer,
                   NULL);

    pthread_join(producerThread, NULL);
    pthread_join(consumerThread, NULL);

    // Destroy synchronization objects
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);

    printf("\nProducer-Consumer execution completed.\n");

    return 0;
}
