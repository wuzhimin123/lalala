#include <timeros/types.h>
#include <timeros/syscall.h>
#include <timeros/string.h>
/*初始进程*/
int main()
{
    sys_exec("user_shell");
}