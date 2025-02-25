#include <timeros/os.h>

extern void frame_allocator_test();
void os_main()
{
   printk("hello xiaozhangye!\n");
   /*trap初始化*/
   frame_allocator_test();
   // trap_init();
   // /*任务初始化，创建任务并设置参数入栈*/
   // task_init();
   // timer_init();
   // run_first_task();
   while(1)
   {

   }
}