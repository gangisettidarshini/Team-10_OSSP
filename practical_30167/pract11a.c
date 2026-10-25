#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 5
#define ITERATIONS 100000

int counter = 0;

void *increment(void *arg)
{
    int i;

    for (i = 0; i < ITERATIONS; i++)
    {
        counter++;
    }

    return NULL;
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int i;

    printf("Race Condition Demonstration\n");
    printf("=============================\n");

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

    return 0;
}
