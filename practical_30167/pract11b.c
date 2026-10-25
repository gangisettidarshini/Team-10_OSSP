#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 5
#define ITERATIONS 100000

int counter = 0;

pthread_mutex_t lock;

void *increment(void *arg)
{
    int i;

    for (i = 0; i < ITERATIONS; i++)
    {
        pthread_mutex_lock(&lock);

        counter++;

        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int i;

    // Initialize mutex
    pthread_mutex_init(&lock, NULL);

    printf("Mutex Synchronization Demonstration\n");
    printf("====================================\n");

    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i],
                       NULL,
                       increment,
                       NULL);
    }

    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("Expected counter value : %d\n",
           NUM_THREADS * ITERATIONS);

    printf("Actual counter value   : %d\n",
           counter);

    // Destroy mutex
    pthread_mutex_destroy(&lock);

    return 0;
}
