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
		/*缺页异常*/
		case 13:/*加载页错误*/
		case 15:/*存储页错误*/
			uint64_t va = r_stval();
			/*懒分配导致*/
			if(uvmshouldtouch())
			{
				struct TaskControlBlock *p = current_proc();
				PhysPageNum mem = kalloc();
				PageTable_map(&p->pagetable,virt_addr_from_size_t(va),
					phys_addr_from_phys_page_num(mem), PAGE_SIZE, 
					PTE_R | PTE_W | PTE_U | PTE_X);
				/*别忘了映射到进程内核页表*/
				kvmcopymappings(p->pagetable,p->kernelpagetable,va,PAGE_SIZE);
			}
			/*写时复制导致*/
			else if(uvmcheckcowpage(r_stval()))
			{
				// uvmcowcopy(r_stval());
				PageTableEntry *pte;
				struct TaskControlBlock *p = current_proc();
				if((pte->bits = find_pte(p->pagetable,virt_page_num_from_size_t(va))) == NULL)//va对应的页表项为空
					panic("uvmcowcopy: find_pte");

				uint64_t pa = PTE2PA(pte->bits);
				uint64_t new;

				if(PA2PGREF(pa) <= 1)//引用计数小于1，无需复制
					new = pa;
				else//否则把父进程的物理页复制到新分配的物理页
				{
					new = size_t_from_phys_page_num(kalloc());
					memcpy((void*)new,(void*)pa,PAGE_SIZE);
					PA2PGREF(pa)--;//引用计数-1
				}
				/*取消对原物理页的映射，无论父子进程,重新映射*/
				uint8_t flags = (PTE_FLAGS(pte->bits) & PTE_W) | ~PTE_COW;//重新映射为可写，清除PTE_COW标志位
				uvmunmap(&p->pagetable,floor_virts(virt_addr_from_size_t(va)),1,0);
				PageTable_map(&p->pagetable,virt_addr_from_size_t(va),
					phys_addr_from_size_t(new), PAGE_SIZE, flags);
				/*别忘了映射到进程内核页表*/
				kvmcopymappings(p->pagetable,p->kernelpagetable,va,PAGE_SIZE);
				// PhysPageNum ppn = kalloc();
            	//ppn转化为物理内存，也就是子进程新分配的物理页的起始地址
            	// u64 paddr = phys_addr_from_phys_page_num(ppn).value;
            	//将旧页的内容拷贝到新页
            	// memcpy((void*)paddr,(void*)phyaddr,PAGE_SIZE);
				
			}
			else
			{
				/*杀死进程*/
			}
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

			cx->sepc += 8;//系统调用完毕要从下一条指令开始运行，若不+8此时sepc设为ecall的地址。
			// trap_handler结束运行__restore将sepc恢复到pc
			/*为什么上面中断不需要，因为中断时自动把sepc设为下一条指令的地址，所以不需要手动设置了*/
			//这里如果是父进程，会得到子进程pid；
			u64 result = __SYSCALL(cx->a7,cx->a0,cx->a1,cx->a2);
			//子进程创建完毕后，处于内核态，父进程swich后到ra寄存器也就是trap_return,然后子进程恢复用户态
			//如果发生了exec，获取新分配的trap上下文地址
			
			cx = get_current_trap_cx();
			cx->a0 = result;
			break;
		default:
			printk("undfined scause:%d\n",scause);
			break;
    	}
	}
	//子进程从此处执行
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
