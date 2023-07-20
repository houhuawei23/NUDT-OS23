#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define SEAT_NUM  2
#define CUSTOMER_NUM  5

/************************************/
sem_t wseat_mtx, cseat_mtx;
sem_t barber_mtx;
sem_t barber_unfinished, barber_finished;

/************************************/

void sleep_random(int t) {
  sleep((int)(t * (rand() / (RAND_MAX *1.0))));
}

void *barber()
{
  int i = 0;
  while(i<5)
  {
/************************************/

    sem_wait(&barber_unfinished);
/************************************/
    printf("barber: start cutting\n");
    sleep_random(3);
    printf("barber: finish cutting\n");
    i++;
/************************************/

    sem_post(&barber_finished);
/************************************/
  }
}

void *customer(void *id)
{
  const int myid = *(int*)id;
  sleep_random(2);
  printf("customer %d: enter waiting-room\n", myid);

/************************************/
    sem_wait(&wseat_mtx);

/************************************/
  printf("customer %d: sit down\n", myid);

/************************************/
   

    sem_wait(&cseat_mtx);
    sem_post(&wseat_mtx);
/************************************/
  printf("customer %d: enter cutting-room and sit down\n", myid);
/************************************/

    sem_wait(&barber_mtx);
    sem_post(&barber_unfinished);
    sem_wait(&barber_finished);
/************************************/
  printf("customer %d: bye\n", myid);
/************************************/
    sem_post(&barber_mtx);
    sem_post(&cseat_mtx);

/************************************/
}

int main()
{
  int i, id[CUSTOMER_NUM];
  pthread_t t[CUSTOMER_NUM];

  srand((int)time(0));
/************************************/
    sem_init(&wseat_mtx, 0, 2);
    sem_init(&cseat_mtx, 0, 1);
    sem_init(&barber_mtx, 0, 1);
    sem_init(&barber_unfinished, 0, 0);
    sem_init(&barber_finished, 0, 0);

/************************************/

  for (i = 0; i < CUSTOMER_NUM; i++)
  {
    id[i] = i + 1;
    pthread_create(&t[i], NULL, customer, &id[i]);
  }
  barber();
  for(i = 0; i < CUSTOMER_NUM; i++){
    pthread_join(t[i], NULL);
  }
  return 0;
}
