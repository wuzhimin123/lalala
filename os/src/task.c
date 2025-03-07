#include <timeros/os.h>

#define USER_STACK_SIZE (4096 * 2)
#define KERNEL_STACK_SIZE (4096 * 2)
#define MAX_TASKS 10
int nextpid = 0;
static int _current = 0;
static int _top = 0;


extern char trampoline[];
uint8_t KernelStack[MAX_TASKS][KERNEL_STACK_SIZE];
uint8_t UserStack[MAX_TASKS][USER_STACK_SIZE]={0};

struct TaskControlBlock tasks[MAX_TASKS];

/*返回一个TaskContext结构体，s0——s11寄存器需要被调用者保存*/
struct TaskContext tcx_init(reg_t kstack_ptr)
{
    struct TaskContext task_ctx;
    /*ra的值设为trap_return，第一次swtich中ret返回__restore，然后回到用户态*/
    task_ctx.ra = trap_return;
    /*sp指向应用上下文*/
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

/*将所有进程状态初始化为UnInit*/
void procinit()
{
    struct TaskControlBlock *p;
    for(p = tasks;p < &tasks[MAX_TASKS];p++)
    {
        p->task_state = UnInit;
    }
}

/* 为每个应用程序映射内核栈*/
void proc_mapstacks(PageTable* kpgtbl)
{
  struct TaskControlBlock *p;
  
  for(p = tasks; p < &tasks[MAX_TASKS]; p++) {
    char *pa = (char*)phys_addr_from_phys_page_num(kalloc()).value;
    if(pa == 0)
      panic("kalloc");
    /*分配时会分配两页，一页是内核栈，一页是保护，具体那一页是内核栈，由本函数最后的赋值决定*/
    u64 va = KSTACK((int) (p - tasks));
    PageTable_map(kpgtbl, virt_addr_from_size_t(va), phys_addr_from_size_t((u64)pa), \
                  PAGE_SIZE, PTE_R | PTE_W);
    // 给某个具体应用的内核栈赋值,指向栈顶，这里指明了分配的两页中哪个是栈顶
    p->kstack = va + PAGE_SIZE;
  }
}

/*为应用程序分配一页内存用来保存Trap上下文*/
void proc_trap(struct TaskControlBlock *p)
{
    /*每个应用程序都有各自的Trap保存页，因此各自分配一页物理内存*/
    p->trap_cx_ppn = phys_addr_from_phys_page_num(kalloc()).value;
    /*初始化任务上下文内存*/
    memset(&p->task_context,0,sizeof(p->task_context));
}

/*为应用程序创建新的根页表，映射trapoline和trap上下文存放地址*/
void proc_pagetable(struct TaskControlBlock *p)
{
    PageTable pagetable;
    //分配了该应用程序的根页表
    pagetable.root_ppn = kalloc();
    //映射跳板页，跳板页都是同一个页
    PageTable_map(&pagetable,virt_addr_from_size_t(TRAMPOLINE),phys_addr_from_size_t((u64)trampoline),\
                    PAGE_SIZE , PTE_R | PTE_X);
    //映射用户程序的trap页（物理地址映射虚拟地址），trap页每个app都不同，各自都有独立的
    PageTable_map(&pagetable,virt_addr_from_size_t(TRAPFRAME),phys_addr_from_size_t(p->trap_cx_ppn), \
                PAGE_SIZE, PTE_R | PTE_W );
    //确定应用的根页表，因为pagetable里存放着根页表号
    p->pagetable = pagetable;
}

/*为应用程序创建根页表，映射trapoline和trap上下文存放地址*/
TaskControlBlock *task_create_pt(size_t app_id)
{
    if(_top < MAX_TASKS)
    {
        proc_trap(&tasks[app_id]);
        proc_pagetable(&tasks[app_id]);
        _top++;
    }

    return &tasks[app_id];
}


/*任务创建后并初始化*/
// void task_create(void (*task_entry)(void))
// {
//     if(_top < MAX_TASKS)
//     {
//         /*TrapContext结构体位于内核栈KernelStack中*/
//         TrapContext *cx_ptr = &KernelStack[_top] + KERNEL_STACK_SIZE - sizeof(TrapContext);
//         reg_t user_sp = &UserStack[_top] + USER_STACK_SIZE;
//         reg_t sstatus = r_sstatus();

//         sstatus &= (0U << 8);
//         w_sstatus(sstatus);
//         /*将sepc设置为task函数地址，使得第一次switch后__restor ret 到task1，
//         */
//         cx_ptr->sepc = (reg_t)task_entry;
//         cx_ptr->sp = user_sp;
//         cx_ptr->sstatus = sstatus;
//         /*任务队列包含任务上下文和任务状态，下面进行初始化*/
//         tasks[_top].task_context = tcx_init((reg_t)cx_ptr);
//         tasks[_top].task_state = Ready;

//         _top++;
//     }
// }

extern u64 kernel_satp;
void app_init(size_t app_id)
{
    //trap_cx_ppn是已经分配的物理内存页，这个代码的意思是
    // 将cx_ptr指向TrapContext结构体，也就是指向已经分配的物理内存页，那么TrapContext便会在分配的内存中创建
    TrapContext* cx_ptr = tasks[app_id].trap_cx_ppn;
    reg_t sstatus = r_sstatus();
    // 设置 sstatus 寄存器第8位即SPP位为0 表示为U模式
    sstatus &= (0U << 8);
    w_sstatus(sstatus);
    // 设置程序入口地址
    cx_ptr->sepc = tasks[app_id].entry;
    // 
    cx_ptr->sstatus = sstatus; 
    // 设置用户栈虚拟地址
    cx_ptr->sp = tasks[app_id].ustack;
    // 设置内核页表token
    cx_ptr->kernel_satp = kernel_satp;
    // 设置内核栈虚拟地址
    cx_ptr->kernel_sp = tasks[app_id].kstack;
    // 设置内核trap_handler的地址
    cx_ptr->trap_handler = (u64)trap_handler;

    /* 构造每个任务任务控制块中的任务上下文，设置 ra 寄存器为 trap_return 的入口地址*/
    tasks[app_id].task_context = tcx_init((reg_t)tasks[app_id].kstack);
    // 初始化 TaskStatus 字段为 Ready
    tasks[app_id].task_state = Ready;
    //分配pid
    tasks[app_id].pid = allocpid();
}

/* 返回当前执行的应用程序的trap上下文的地址 */
u64 get_current_trap_cx()
{
  return tasks[_current].trap_cx_ppn;
}

/* 返回当前执行的应用程序的satp token*/
u64 current_user_token()
{
   return MAKE_SATP(tasks[_current].pagetable.root_ppn.value);
}

void run_first_task()
{
    tasks[0].task_state = Running;
    struct TaskContext *next_task_cx_ptr = &(tasks[0].task_context);
    struct TaskContext _unused;
    printf("run_first_task\n");
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
    // tasks[_current].task_state = Ready;
    /*Ready才可以切换*/
    if(tasks[next].task_state == Ready || tasks[next].task_state == Running)
    {
        struct TaskContext *current_task_cx_ptr = &(tasks[_current].task_context);
        struct TaskContext *next_task_cx_ptr = &(tasks[next].task_context);
        tasks[next].task_state = Running;
        _current = next;
        __switch(current_task_cx_ptr,next_task_cx_ptr);
    }
}

/*返回当前进程PCB指针*/
struct TaskControlBlock *current_proc()
{
    return &tasks[_current];
}

/*分配pid号*/
int allocpid()
{
    int pid;
    pid = nextpid;
    nextpid = nextpid + 1;
    return pid;
}

/*为进程映射Trap页和跳板页*/
struct TaskControlBlock* allocproc()
{
    struct TaskControlBlock *p;
    //任务列表放着MAX_TASKS多个任务，只不过是有些没有"激活"，
    // 这里拿到一个没有激活的任务作为新任务
    for(p = tasks;p < &tasks[MAX_TASKS];p++)
    {
        if(p->task_state == UnInit)
            goto found;
    }
    return 0;
found:
       p->pid = allocpid();
       p->task_state = Ready;
       //为每个应用分配一页内存来存放trap上下文
       proc_trap(p);
       //为每个用户程序创建页表，映射跳板页和trap上下文页
       proc_pagetable(p);
    return p;
}

/*分配子进程*/
int __sys_fork()
{
    struct TaskControlBlock *np;
    struct TaskControlBlock *p = current_proc();

    if((np = allocproc()) == 0)
        return -1;
    //拷贝父进程内存数据
    uvmcopy(&p->pagetable,&np->pagetable,p->base_size);
    //拷贝父进程trap页数据
    memcpy((void*)np->trap_cx_ppn,(void*)p->trap_cx_ppn,PAGE_SIZE);
    TrapContext *cx_ptr = np->trap_cx_ppn;
    //子进程返回0，修改应用内核栈的a0，后面restore恢复到a0寄存器中返回0
    cx_ptr->a0 = 0;
    //在trap页中设置子进程内核栈，因为之前是复制的父进程的
    cx_ptr->kernel_sp = np->kstack;
    //复制TCB的信息
    np->entry = p->entry;
    np->base_size = p->base_size;
    np->parent = p;
    np->ustack = p->ustack;
    //设置子进程返回地址和内核栈
    np->task_context = tcx_init((reg_t)np->kstack);

    _top++;
    return np->pid;

}
/*当前进程下，释放旧app页表，为新app创建新页表并映射*/
int exec(const char* name)
{
    AppMetadata metadata = get_app_data_by_name(name);
    //ELF 文件头
    elf64_ehdr_t *ehdr = metadata.start;
    //检查elf文件
    elf_check(ehdr);
    //获取当前进程
    struct TaskControlBlock* proc = current_proc();
    //保存旧的页表
    PageTable old_pagetable = proc->pagetable;
    //旧进程用到的内存大小
    u64 oldsz = proc->base_size;
    //在原来进程中重新分配页表
    proc_pagetable(proc);
    //加载程序段并映射
    load_segment(ehdr,proc);
    //映射用户栈开始地址
    proc_ustack(proc);
    //拿到trap上下文页物理地址,开始初始化赋值
    TrapContext* cx_ptr = proc->trap_cx_ppn;
    cx_ptr->sepc = (u64)ehdr->e_entry;
    cx_ptr->sp = proc->ustack;
    /*下面这些还是用的当前进程的Trap上下文，只是重新映射，以下内容不需要改变，*/
    // reg_t sstatus = r_sstatus();
    // //设置第8位spp位位0，表示为U模式
    // sstatus &= (0U << 8);
    // w_sstatus(sstatus);
    // cx_ptr->sstatus = sstatus;
    // //内核页表token
    // cx_ptr->kernel_satp = kernel_satp;
    // //内核栈虚拟地址
    // cx_ptr->kernel_sp = proc->kstack;
    // //trap_handler地址
    // cx_ptr->trap_handler = (u64)trap_handler;
    //释放旧应用程序旧页表
    proc_freepagetable(&old_pagetable,oldsz);
    printk("sys_exec\n");
    return 0;
}

/*进程回收恢复全新*/
void freeproc(struct TaskControlBlock *p)
{
    proc_freepagetable(&p->pagetable,p->base_size);
    p->pagetable.root_ppn.value = 0;
    p->base_size = 0;
    p->parent =  0;
    p->ustack = 0;
    p->entry = 0;
    p->task_state = UnInit;
    p->exit_code = 0;
}

void exit_current_and_run_next(u64 exit_code)
{
    struct TaskControlBlock *p = current_proc();
    if(p->pid == 0)
        panic("init exiting");

        p->exit_code = exit_code;
        p->task_state = Zombie;
        /*将子进程都挂在初始进程下面*/
        children_proc_clear(p);
        /*进程数量减一*/
        _top--;
        schedule();
}

int wait()
{
    struct TaskControlBlock *children;
    struct TaskControlBlock *p = current_proc();
    int pid,havekids;
    /**/
    for(;;)
    {
        havekids = 0;
        for(children = tasks;children < &tasks[MAX_TASKS];children++)
        {
            if(children->parent == p)
            {
                havekids = 1;
                if(children->task_state == Zombie)
                {
                    pid = children->pid;
                    freeproc(children);
                    printk("child pid:%d\n",pid);
                    return pid;
                }
            }
        }
        //没有子进程
        if(!havekids)
            return -1;
        //有非Zombie的子进程，去执行别的进程，
        // 然后从别的进程返回来继续检测是否变为Zobie状态
        schedule();
    }
}

/*把子进程挂在initproc进程下面*/
void children_proc_clear(struct TaskControlBlock *p)
{
    struct TaskControlBlock *children;
    for(children = tasks; children < &tasks[MAX_TASKS]; children++)
    {
        if(children->parent == p)
            children->parent = &tasks[0];
    }
}