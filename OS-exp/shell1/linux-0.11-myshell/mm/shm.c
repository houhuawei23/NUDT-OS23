#include <linux/kernel.h>
#include <string.h>
#include <linux/mm.h>
#include <errno.h>
#include <unistd.h>
#include <linux/sched.h>
#include <linux/shm.h>

//共享内存的头节点
struct shm_node *shmHead =&((struct shm_node *){0, 0, 0, NULL, NULL});
/**
 * 根据key，获取或新建一片共享内存。
 */ 
int sys_shmget(key_t key, size_t size, int shmflg)
{  
    struct shm_node *tmp = shmHead;
    //检查参数
    if(size > PAGE_SIZE || key == 0)    //最多分配4KB内存,且key不能为0，这是被头节点占用的
    {
        errno = EINVAL;
        return(-1);
    }
    //查找链表
    while(tmp != NULL)
    {
        if(key == tmp->key)
        {
            return(tmp->id);
        }
        tmp = tmp->next;
    }
    //若没有找到则分配一页新的内存
    tmp = (struct shm_node *)malloc(sizeof(struct shm_node));
    //分配一页内存。get_free_page()返回的是空闲内存的物理地址，
    //不过在内核空间中：偏移地址(32bit) = 线性地址(32bit) = 物理地址(32bit)
    tmp->addr = get_free_page();   
    printk("tmp->addr = %d\n", tmp->addr);
    if(tmp->addr == 0)
    {
        errno = ENOMEM;
        return(-1);
    }
    tmp->key = key;    //shmid和key都是用来标识共享内存的，因此为了简化，
    tmp->id = (int)key;//这里直接将shmid = key。
    tmp->size = size;
    tmp->next = shmHead->next;
    shmHead->next = tmp;
    printk("sys_shmget succeed !\n");
    return(tmp->id);
}
/**
 * 将shmid指定的共享页面映射到当前进程的虚拟地址空间中，并将其首地址返回。
 * 注意这里返回的是偏移地址。
 */ 
void *sys_shmat(int shmid, const void *shmaddr, int shmflg)
{
    struct shm_node *tmp = shmHead;
    unsigned long address = 0;   //线性地址
    //检查参数
    while(tmp != NULL)
    {
        if(shmid == tmp->id)
            break;
        tmp = tmp->next;
    }
    if(tmp == NULL)
    {
        errno = EINVAL;
        return(-1);
    }
    //寻找空闲的虚拟地址空间
    //current->start_code + current->brk就是我们要的空闲线性地址
    printk("start_code = %d, brk = %d\n", current->start_code, current->brk);
    printk("data base = %d\n", get_base(current->ldt[2]) );
    address = current->start_code + current->brk;
    if( 0 == put_page(tmp->addr, address) )
    {
        printk("put_page fail !\n");
        return(NULL);
    }
    current->brk += PAGE_SIZE;
    printk("sys_shmat succeed !\n");
    //注意：address是线性地址，不能直接返回，我们要返回的是偏移地址
    //return((void *)(address));
    return(address - current->start_code);
}
