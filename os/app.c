#include <stddef.h>
#include "os.h"
#include "context.h"
#include "string.h"




/*系统调用，系统调用号和参数*/
size_t syscall(size_t id, reg_t arg1, reg_t arg2, reg_t arg3) {
    long ret;
    asm volatile (
        "mv a7, %1\n\t"   // Move syscall id to a0 register
        "mv a0, %2\n\t"   // Move args[0] to a1 register
        "mv a1, %3\n\t"   // Move args[1] to a2 register
        "mv a2, %4\n\t"   // Move args[2] to a3 register
        "ecall\n\t"       // Perform syscall
        "mv %0, a0"       // Move return value to 'ret' variable
        : "=r" (ret)
        : "r" (id), "r" (arg1), "r" (arg2), "r" (arg3)
        : "a7", "a0", "a1", "a2", "memory"
    );
    return ret;
}

size_t sys_write(size_t fd, const char *buf, size_t len)
{
    syscall(__NR_write, fd, buf, len);
}

size_t sys_yield()
{
    syscall(__NR_sched_yield,0,0,0);
}

void task_delay(volatile int count)
{
    count *=50000;
    while(count--);
}

uint64_t sys_gettime()
{
    return syscall(__NR_gettimeofday,0,0,0);
}

void task1()
{
    const char *message = "task1 is running!\n";
    int len = strlen(message);
    while(1)
    {
        sys_write(1,message,len);
    }
}

void task2()
{
    const char *message = "task2 is running!\n";
    int len = strlen(message);
    while(1)
    {
        sys_write(1,message,len); 
    }
}

void task3()
{
    const char *message = "task3 is running!\n";
    int len = strlen(message);
    while(1)
    {
        sys_write(1,message,len);
    }
}

void task_init(void)
{
    task_create(task1);
    task_create(task2);
    task_create(task3);
}

