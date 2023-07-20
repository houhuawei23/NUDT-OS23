#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define LIMIT 20
#define M 5
#define N 8
/************************************/
int buffer1[M];
int buffer2[N];
sem_t full1, empty1, full2, empty2;
/************************************/
void sleep_random(int t)
{
    sleep((int)(t * (rand() / (RAND_MAX * 1.0))));
}

void *P()
{
    int i;
    for (i = 0; i < LIMIT; i++)
    {
        sleep_random(2);
        /************************************/
        sem_wait(&empty1);
        buffer1[i % M] = i;
        printf("P sends:\t%d\n", i);
        sem_post(&full1);
        /************************************/
    }
}

void *Q()
{
    int i, data;
    for (i = 0; i < LIMIT; i++)
    {
        sleep_random(2);
        /************************************/
        sem_wait(&full1);
        data = buffer1[i % M];
        sem_post(&empty1);
        sem_wait(&empty2);
        buffer2[i % N] = data;
        sem_post(&full2);
        /************************************/
    }
}

void *R()
{
    int i;
    for (i = 0; i < LIMIT; i++)
    {
        sleep_random(2);
        /************************************/
        sem_wait(&full2);
        printf("R receives: %d\n", buffer2[i % N]);
        sem_post(&empty2);
        /************************************/
    }
}

int main()
{
    int i;
    pthread_t t1, t2;
    for (i = 0; i < M; i++)
        buffer1[i] = -1;
    for (i = 0; i < N; i++)
        buffer2[i] = -1;
    srand((int)time(0));
    /************************************/
    sem_init(&full1, 0, 0);
    sem_init(&empty1, 0, M);
    sem_init(&full2, 0, 0);
    sem_init(&empty2, 0, N);
    /************************************/
    pthread_create(&t1, NULL, P, NULL);
    pthread_create(&t2, NULL, Q, NULL);
    R();
    return 0;
}
