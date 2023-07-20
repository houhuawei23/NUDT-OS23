#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define N 10
#define PRODUCT_NUM 15
int buffer[N], readpos = 0, writepos = 0;
sem_t full, empty, mutexp, mutexc;

void sleep_random(int t)
{
    sleep((int)(t * (rand() / (RAND_MAX * 1.0))));
}

void *produce()
{
    int i;
    int tid;
    tid = gettid();
    for (i = 0; i < PRODUCT_NUM; i++)
    {
        sleep_random(2);
        sem_wait(&empty);
        sem_wait(&mutexp);
        buffer[writepos++] = i + 1;
        if (writepos >= N)
            writepos = 0;
        printf("tid: %d\t, produce: %d\n", tid, i + 1); 
        // printf("produce:    %d\n", i + 1);
        sem_post(&mutexp);
        sem_post(&full);
    }
}

void *consume()
{
    int i;
    int tid;
    tid = gettid();
    for (i = 0; i < PRODUCT_NUM; i++)
    {
        sleep_random(2);
        sem_wait(&full);
        sem_wait(&mutexc);
        printf("tid: %d\t, consume: %d\n", tid, buffer[readpos]);
        // printf("consume: %d\n", buffer[readpos]);
        buffer[readpos++] = -1;
        if (readpos >= N)
            readpos = 0;
        sem_post(&mutexc);
        sem_post(&empty);
    }
}

int main()
{
    int resp[5], resc[5], i;
    pthread_t tp[5], tc[5];
    for (i = 0; i < N; i++)
        buffer[i] = -1;
    srand((int)time(0));
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, N);
    sem_init(&mutexp, 0, 1);
    sem_init(&mutexc, 0, 1);
    // res = pthread_create(&t1, NULL, produce, NULL);
    // if (res != 0){
    //   perror("failed to create thread");
    //   exit(1);
    // }
    // consume();
    for (i = 0; i < 5; i++)
    {
        resp[i] = pthread_create(&tp[i], NULL, produce, NULL);
        if (resp[i] != 0)
        {
            perror("failed to create thread");
            exit(1);
        }
    }
    for (i = 0; i < 5; i++)
    {
        resc[i] = pthread_create(&tc[i], NULL, consume, NULL);
        if (resc[i] != 0)
        {
            perror("failed to create thread");
            exit(1);
        }
    }
    for (i = 0; i < 5; i++)
    {
        pthread_join(tp[i], NULL);
        pthread_join(tc[i], NULL);
    }
    return 0;
}
