#include <timeros/os.h>

void trap_from_kernel()
{
	panic("a trap from kernel!\n");
}

void set_kernel_trap_entry()
{
	w_stvec((reg_t)trap_from_kernel);
}
/*设置user Trap的地址*/
void set_user_trap_entry()
{
	w_stvec((reg_t)TRAMPOLINE);  
}

void trap_handler()
{
	/*对于处于S态触发Trap的简单处理*/
	set_kernel_trap_entry();
	//获得当前应用的上下文地址
	TrapContext* cx = get_current_trap_cx();
	printk("cx地址:%p\n",(void*)cx);
    reg_t scause = r_scause();
	reg_t cause_code = scause & 0xfff;//提取低12位，记录Trap具体原因
	if(scause & 0x8000000000000000)//1<<63 = x8000000000000000,判断中断还是异常
	{
		switch (cause_code)
		{
		/*rtc中断*/
		case 5:
			set_next_trigger();
			schedule();//中断发生则切换任务
			break;
		default:
			printk("undfined scause:%d\n",scause);
			break;
    	}
	}
	else
	{
		switch (cause_code)
		{
		/*用户态的系统调用*/
		case 8:
			cx->a0 = __SYSCALL(cx->a7,cx->a0,cx->a1,cx->a2);
			cx->sepc += 8;//系统调用完毕要从下一条指令开始运行，若不+8此时sepc设为ecall的地址。
			// trap_handler结束运行__restore将sepc恢复到pc
			/*为什么上面中断不需要，因为中断时自动把sepc设为下一条指令的地址，所以不需要手动设置了*/
			break;
		default:
			printk("undfined scause:%d\n",scause);
			break;
    	}
	}
	//__restore之前设置好Trap上下文保存地址等
    trap_return();
}

/*__restore之前的设置*/
void trap_return()
{
	//将stvec设置为内核和应用地址空间共享的跳板页面起始地址（虚拟地址）
	set_user_trap_entry();
	//Trap上下文在应用地址空间的虚拟地址
	u64 trap_cx_ptr = TRAPFRAME;
	//获得应用地址空间satp应设为的值
	u64 user_satp = current_user_token();
	//__restore的虚拟地址
	u64 restore_va = (u64)__restore - (u64)__alltraps + TRAMPOLINE;
	//内联汇编
	asm volatile (    
		"fence.i\n\t"    
		"mv a0, %0\n\t"  // 将trap_cx_ptr传递给a0寄存器  
		"mv a1, %1\n\t"  // 将user_satp传递给a1寄存器  
		"jr %2\n\t"      // 跳转到restore_va的位置执行代码  
		:    
		: "r" (trap_cx_ptr),    
		"r" (user_satp),
		"r" (restore_va)
		: "a0", "a1"
	);
}
