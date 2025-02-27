#include <timeros/os.h>
#include <timeros/syscall.h>
#include <timeros/stdio.h>

int main(int argc, char const *argv[])
{
    uint64_t current_timer = 0;
    while (1)
    {
       current_timer = sys_gettime();
       printf("current_timer:%x\n",current_timer);

    }
    return 0;
}