#include "os.h"


void os_main()
{
   printf("hello xiaozhangye!\n");
   /*trap初始化*/
   trap_init();
   /*任务初始化，创建任务并设置参数入栈*/
   task_init();
   run_first_task();
}