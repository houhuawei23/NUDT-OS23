#ifndef _SYS_SHM_H
#define _SYS_SHM_H

#include <linux/sched.h>

typedef int key_t;
/*共享内存节点*/
struct shm_node{
    key_t key;
    int id;           
    unsigned int size;
    unsigned long addr;      /*物理地址*/
    struct shm_node *next;   /*下一个节点*/
};
extern int errno;
#endif