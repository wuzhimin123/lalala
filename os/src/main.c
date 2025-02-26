#include <timeros/os.h>
#include <timeros/assert.h>
extern void frame_alloctor_init();
extern void kvminit();
extern void kvminithart();
void os_main()
{
   printk("hello timer os!\n");

   // 内存分配器初始化
   frame_alloctor_init();
   //内存初始化，将内核部分映射
   kvminit();

   //切换到内核页表
   kvminithart();
   //Trap初始化
   trap_init();

   while(1)
   {
      
   }
   

   // task_init();

   // timer_init();

   
   // run_first_task();

}