#include <timeros/os.h>


void os_main()
{
   // printk("hello timer os!\n");

   // 内存分配器初始化（栈式内存分配，指定128m为可分配内存，current 和 end 对应kernelend和phystop）
   frame_alloctor_init();
   
   //初始化内存
   kvminit();
   //初始化进程
   procinit();
   //加载进程(initproc)
   load_app(0);
   app_init(0);
   
   //开启sv39模式
   kvminithart();

   //trap初始化
   set_kernel_trap_entry();

   get_app_names();
   
   timer_init();
   
   printk("os!\n");
   run_first_task();
   printk("hello timer os!\n");
}