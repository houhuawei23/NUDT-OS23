#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define SEAT_NUM  2
#define CUSTOMER_NUM  5

/************************************/

int buffer[SEAT_NUM];
sem_t seat_empty, seat_full, barber_empty, barber_full;
/************************************/

void sleep_random(int t) {
  sleep((int)(t * (rand() / (RAND_MAX *1.0))));
}

void *barber()
{
  while(5)
  {
/************************************/
    sem_wait(&barber_full);
/************************************/
    printf("barber: start cutting\n");
    sleep_random(3);
    printf("barber: finish cutting\n");
/************************************/
    sem_post(&barber_empty);

/************************************/
  }
}

void *customer(void *id)
{
  const int myid = *(int*)id;
  sleep_random(2);
  printf("customer %d: enter waiting-room\n", myid);

/************************************/
    sem_wait(&seat_empty);
/************************************/
  printf("customer %d: sit down\n", myid);

/************************************/
    sem_post(&seat_full);
    
/************************************/
  printf("customer %d: enter cutting-room and sit down\n", myid);
/************************************/
    sem_wait(&barber_empty);
    
/************************************/
  printf("customer %d: bye\n", myid);
/************************************/
    sem_post(&barber_full);

/************************************/
}

int main()
{
  int i, id[CUSTOMER_NUM];
  pthread_t t[CUSTOMER_NUM];

  srand((int)time(0));
/************************************/
    sem_init(&seat_empty, 0, SEAT_NUM);
    sem_init(&seat_full, 0, 0);
    sem_init(&barber_empty, 0, 1);
    sem_init(&barber_full, 0, 0);
/************************************/

  for (i = 0; i < CUSTOMER_NUM; i++)
  {
    id[i] = i + 1;
    pthread_create(&t[i], NULL, customer, &id[i]);
  }
  barber();
  return 0;
}
