#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define SEAT_NUM  2
#define CUSTOMER_NUM  5

/************************************/

sem_t wseat_empty, wseat_full, cseat_empty, cseat_full;
sem_t barber_mutex, barber_unfinished, barber_finished;

/************************************/

void sleep_random(int t) {
  sleep((int)(t * (rand() / (RAND_MAX *1.0))));
}

void *barber()
{
  while(5)
  {
/************************************/

    sem_wait(&barber_unfinished);
/************************************/
    printf("barber: start cutting\n");
    sleep_random(3);
    printf("barber: finish cutting\n");
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
    sem_wait(&wseat_empty);//

/************************************/
  printf("customer %d: sit down\n", myid);

/************************************/

    sem_post(&wseat_full);//

    sem_wait(&cseat_empty);
/************************************/
  printf("customer %d: enter cutting-room and sit down\n", myid);
/************************************/
    sem_post(&cseat_full);

    sem_post(&wseat_empty);//

    sem_wait(&barber_mutex);
    sem_post(&barber_unfinished);
    sem_wait(&barber_finished);
/************************************/
  printf("customer %d: bye\n", myid);
/************************************/
    
    // sem_post(&barber_mutex);
    sem_post(&cseat_empty);

/************************************/
}

int main()
{
  int i, id[CUSTOMER_NUM];
  pthread_t t[CUSTOMER_NUM];

  srand((int)time(0));
/************************************/
    sem_init(&wseat_empty, 0, SEAT_NUM);
    sem_init(&wseat_full, 0, 0);
    sem_init(&cseat_empty, 0, 1);
    sem_init(&cseat_full, 0, 0);

    sem_init(&barber_mutex, 0, 1);

    sem_init(&barber_finished, 0, 0);
    sem_init(&barber_unfinished, 0, 0);
/************************************/

  for (i = 0; i < CUSTOMER_NUM; i++)
  {
    id[i] = i + 1;
    pthread_create(&t[i], NULL, customer, &id[i]);
  }
  barber();
  return 0;
}
