#include "os.h"

#define USER_STACK_SIZE (4096 * 2)
#define KERNEL_STACK_SIZE (4096 * 2)
#define MAX_TASKS 10
static int _current = 0;
static int _top = 0;

uint8_t KernelStack[MAX_TASKS][KERNEL_STACK_SIZE];
uint8_t UserStack[MAX_TASKS][USER_STACK_SIZE]={0};

struct TaskControlBlock tasks[MAX_TASKS];

/*返回一个TaskContext结构体，s0——s11寄存器需要被调用者保存*/
struct TaskContext tcx_init(reg_t kstack_ptr)
{
    struct TaskContext task_ctx;
    /*ra的值设为__restore，第一次swtich中ret返回__restore，然后回到用户态*/
    task_ctx.ra = __restore;
    /*sp指向TrapContext结构体*/
    task_ctx.sp = kstack_ptr;
    task_ctx.s0 = 0;
    task_ctx.s1 = 0;
    task_ctx.s2 = 0;
    task_ctx.s3 = 0;
    task_ctx.s4 = 0;
    task_ctx.s5 = 0;
    task_ctx.s6 = 0;
    task_ctx.s7 = 0;
    task_ctx.s8 = 0;
    task_ctx.s9 = 0;
    task_ctx.s10 = 0;
    task_ctx.s11 = 0;

    return task_ctx;
}
/*任务创建后并初始化*/
void task_create(void (*task_entry)(void))
{
    if(_top < MAX_TASKS)
    {
        /*TrapContext结构体位于内核栈KernelStack中*/
        TrapContext *cx_ptr = &KernelStack[_top] + KERNEL_STACK_SIZE - sizeof(TrapContext);
        reg_t user_sp = &UserStack[_top] + USER_STACK_SIZE;
        reg_t sstatus = r_sstatus();

        sstatus &= (0U << 8);
        w_sstatus(sstatus);
        /*将sepc设置为task函数地址，使得第一次switch后__restor ret 到task1，
        */
        cx_ptr->sepc = (reg_t)task_entry;
        cx_ptr->sp = user_sp;
        cx_ptr->sstatus = sstatus;
        /*任务队列包含任务上下文和任务状态，下面进行初始化*/
        tasks[_top].task_context = tcx_init((reg_t)cx_ptr);
        tasks[_top].task_state = Ready;

        _top++;
    }
}

void run_first_task()
{
    tasks[0].task_state = Running;
    struct TaskContext *next_task_cx_ptr = &(tasks[0].task_context);
    struct TaskContext _unused;
    /*将上面初始化好的当前任务上下文和下一个任务上下文传递给__switch，__switch作为被调用者需要保存s0——s11寄存器*/
    __switch(&_unused,next_task_cx_ptr);
    panic("unreachable in run_first_task!");
}
/*任务切换核心*/
void schedule()
{
    if(_top <= 0)
    {
        panic("Num of task should be greater than zero!\n");
        return;
    }

    /*轮转调度*/
    int next = _current + 1;
    next = next % _top;
    /*Ready才可以切换*/
    if(tasks[next].task_state == Ready)
    {
        struct TaskContext *current_task_cx_ptr = &(tasks[_current].task_context);
        struct TaskContext *next_task_cx_ptr = &(tasks[next].task_context);
        tasks[next].task_state = Running;
        tasks[_current].task_state = Ready;
        _current = next;
        __switch(current_task_cx_ptr,next_task_cx_ptr);
    }
}