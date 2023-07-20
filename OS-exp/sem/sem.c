#include <linux/sem.h>
#include <linux/sched.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>
#include <signal.h>
sem_t semtable[SEM_TABLE_LEN];

sem_t *sem_open(const char *name, unsigned int value);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_unlink(const char *name);

int sem_cnt = 0;
sem_t *sys_sem_open(const char *name, unsigned int value)
{
    if(sem_cnt > SEM_TABLE_LEN - 1){
        return NULL;
    }
    char kernelname[50];
    int i = 0;
    int name_len = 0;
    sem_t* p;
    while (get_fs_byte(name + name_len) != '\0')
        name_len ++;
    if( name_len > SEM_NAME_LEN)
        return NULL;
    for(i = 0; i < name_len; i++){
        kernelname[i] = get_fs_byte(name + i);
    }

    int isExist = 0;
    for(i = 0; i< sem_cnt; i++){
        if(!strcmp(name, semtable[i].name)){
            isExist = 1;
            break;
        }
    }

    if(isExist){
        p = (sem_t*)(&semtable[i]);
    }
    else{
        for(i = 0; i <name_len; i++){
            semtable[sem_cnt].name[i] = kernelname[i];
        }
        semtable[sem_cnt].value = value;
        p = (sem_t*)(&semtable[sem_cnt]);
        sem_cnt ++;
    }
    return p;
}

int sys_sem_wait(sem_t *sem){
    cli();
    sem->value--;
    if((sem->value) < 0){
        sleep_on(&(sem->queue));
    }
    sti();
    return 0;
}
int sys_sem_post(sem_t *sem){
    cli();
    sem->value ++;
    if((sem->value) <=0){
        wake_up(&(sem->queue));
    }
    sti();
    return 0;
}
int sys_sem_unlink(const char *name){
    if(sem_cnt > SEM_TABLE_LEN - 1){
        return -1;
    }

    char kernelname[50];
    int i = 0;
    int name_len = 0;

    while (get_fs_byte(name + name_len) != '\0')
        name_len ++;
    if( name_len > SEM_NAME_LEN)
        return -1;
    for(i = 0; i < name_len; i++){
        kernelname[i] = get_fs_byte(name + i);
    }

    int isExist = 0;
    for(i = 0; i< sem_cnt; i++){
        if(!strcmp(name, semtable[i].name)){
            isExist = 1;
            break;
        }
    }
    if(isExist){
        int j = 0;
        for(j = i; j < sem_cnt; j++){
            semtable[j]=semtable[j+1];
        }
        sem_cnt --;
        return 0;
    }
    else{
        return -1;
    }
}
