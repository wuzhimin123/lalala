#include <timeros/os.h>
#include <timeros/syscall.h>
#include <timeros/string.h>

int main(int argc, char const *argv[])
{
    int pid = sys_fork();
    while (1)
    {
        // //父进程
        // if(pid > 0)
        //     printf("father\n");
        // else if(pid == 0)
        //     printf("child\n");
        // else
        //     printf("failed!\n");
        printf("pid:%d\n",pid);

    }
    return 0;
}