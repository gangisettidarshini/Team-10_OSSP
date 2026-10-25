#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t resource1;
pthread_mutex_t resource2;

void *thread1(void *arg)
{
    printf("Thread 1 started\n");

    printf("Thread 1 requesting Resource 1\n");
    pthread_mutex_lock(&resource1);

    printf("Thread 1 acquired Resource 1\n");

    sleep(1);

    printf("Thread 1 requesting Resource 2\n");
    pthread_mutex_lock(&resource2);

    printf("Thread 1 acquired Resource 2\n");

    printf("Thread 1 is using both resources\n");

    pthread_mutex_unlock(&resource2);
    pthread_mutex_unlock(&resource1);

    printf("Thread 1 released resources\n");

    return NULL;
}

void *thread2(void *arg)
{
    printf("Thread 2 started\n");

    // IMPORTANT:
    // Thread 2 also requests Resource 1 first.
    // This follows the same resource ordering.

    printf("Thread 2 requesting Resource 1\n");
    pthread_mutex_lock(&resource1);

    printf("Thread 2 acquired Resource 1\n");

    sleep(1);

    printf("Thread 2 requesting Resource 2\n");
    pthread_mutex_lock(&resource2);

    printf("Thread 2 acquired Resource 2\n");

    printf("Thread 2 is using both resources\n");

    pthread_mutex_unlock(&resource2);
    pthread_mutex_unlock(&resource1);

    printf("Thread 2 released resources\n");

    return NULL;
}

int main()
{
    pthread_t t1, t2;

    pthread_mutex_init(&resource1, NULL);
    pthread_mutex_init(&resource2, NULL);

    printf("Deadlock Prevention Using Resource Ordering\n");
    printf("============================================\n\n");

    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&resource1);
    pthread_mutex_destroy(&resource2);

    printf("\nProgram completed without deadlock.\n");

    return 0;
}
