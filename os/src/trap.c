#include <timeros/os.h>

TrapContext* trap_handler(TrapContext* cx)
{
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
			printf("undfined scause:%d\n",scause);
			break;
    	}
	}
	else
	{
		switch (scause)
		{
		/*用户态的系统调用*/
		case 8:
			__SYSCALL(cx->a7,cx->a0,cx->a1,cx->a2);
			cx->sepc += 8;//系统调用完毕要从下一条指令开始运行，若不+8此时sepc设为ecall的地址。
			// trap_handler结束运行__restore将sepc恢复到pc
			/*为什么上面中断不需要，因为中断时自动把sepc设为下一条指令的地址，所以不需要手动设置了*/
			break;
		default:
			printf("undfined scause:%d\n",scause);
			break;
    	}
	}
    return cx;
}
void trap_init()
{
    /*设置ecall进入地址*/
    w_stvec((reg_t)__alltraps);
}