
     .section .text.entry #定义当前代码所在段
     .globl _start  #声明全局符号，链接器可以找到并将其设为程序起始执行地址
_start:
    la sp, boot_stack_top   #将栈顶地址加载到sp
    call os_main    #函数调用

    .section .bss.stack #定义一个.bss段用来存放栈数据
    .globl boot_stack_lower_bound #声明全局符号
boot_stack_lower_bound:
    .space 4096 * 16 #在内存的当前位置预留64KB栈空间
    .globl boot_stack_top
boot_stack_top: