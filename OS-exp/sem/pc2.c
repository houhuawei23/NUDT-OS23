#define __LIBRARY__
#include <stdio.h>
#include <unistd.h>
#include <linux/sem.h>
#include <fcntl.h>

_syscall2(sem_t *, sem_open, const char *, name, unsigned int, value);
_syscall1(int, sem_wait, sem_t *, sem);
_syscall1(int, sem_post, sem_t *, sem);
_syscall1(int, sem_unlink, const char *, name);

const int BUFFER_SIZE = 10;
const int ITEM_COUNT = 500;
const int PRODUCER_COUNT = 1;
const int CONSUMER_COUNT = 5;

const char *FILE_NAME = "buffer_file";

sem_t *empty, *full, *mutex;
int item_pro, item_used;
int fw, fr;

int main(int argc, char *argv[])
{

    /* char *filename; */
    int pid;
    int i;
    fw = open(FILE_NAME, O_CREAT | O_TRUNC | O_WRONLY, 0222); /* 以只写方式打开文件给生产者写入产品编号 */
    fr = open(FILE_NAME, O_TRUNC | O_RDONLY, 0444);           /* 以只读方式打开文件给消费者读出产品编号 */

    empty = sem_open("EMPTY", BUFFER_SIZE);
    full = sem_open("FULL", 0);
    mutex = sem_open("METUX", 1);

    for (i = 0; i < CONSUMER_COUNT; i++)
    {
        pid = fork();

        if (pid == 0)
        {
            pid = getpid();
            printf("consumer pid: %d\n", pid);
            fflush(stdout);

            while (1)
            {
                sem_wait(full);
                sem_wait(mutex);
                /*  读写文件 */

                /* read(fr, (char *)&item_used, sizeof(int)); */
                if (!read(fr, (char *)&item_used, sizeof(item_used)))
                {
                    printf("pid: %d \t read at the end\n", pid);
                    lseek(fr, 0, 0);
                    read(fr, (char *)&item_used, sizeof(item_used));
                }
                printf("pid %d:\t consumes item %d\n", pid, item_used);
                fflush(stdout);

                sem_post(empty); /* 唤醒生产者进程 */
                sem_post(mutex);

                if (item_used == ITEM_COUNT)
                    goto OK;
            }
        }
    }
    /*  生产者进程 */
    pid = getpid();
    printf("producer pid: %d\n", pid);
    for (item_pro = 0; item_pro < ITEM_COUNT; item_pro++)
    {
        sem_wait(empty);
        sem_wait(mutex);
        /*  读写文件 */
        if (!(item_pro % BUFFER_SIZE)){
            printf("pid: %d \t, item_pro: %d \t write at the end\n", pid, item_pro);
            lseek(fw, 0, 0);
        }
        write(fw, (char *)&item_pro, sizeof(int));

        printf("pid %d:\tproduces item %d\n", pid, item_pro);
        fflush(stdout);
        sem_post(mutex);
        sem_post(full);
    }
    OK:
        close(fw);
        close(fr);
        return 0;
}