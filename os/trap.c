#include "os.h"
#include "context.h"
#include "riscv.h"

TrapContext* trap_handler(TrapContext* cx)
{
    reg_t scause = r_scause();
	switch (scause)
	{
	case 8:
			__SYSCALL(cx->a7,cx->a0,cx->a1,cx->a2);
		break;
	default:
			printf("undfined scause:%d\n",scause);
			//panic("error!");
		break;
    }
    /*恢复到下一个指令继续执行*/
    cx->sepc += 8;
    return cx;
}
void trap_init()
{
    /*设置ecall进入地址*/
    w_stvec((reg_t)__alltraps);
}