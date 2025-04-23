#include <timeros/os.h>
#include <timeros/stdio.h>
#include <timeros/address.h>
//转换为物理地址(应用层和内核层看到的地址不同，需要转化)
char *translated_byte_buffer(const char* data)
{   
    u64 user_satp = current_user_token();  
    PageTable  pt ;
    //satp的值转换为根物理页号
    pt.root_ppn.value = MAKE_PAGETABLE(user_satp);
    //指针data转换为虚拟地址
    u64 start_va = data;
    // u64 end_va = start_va + len;
    //最终找到页表项
    VirtPageNum vpn = floor_virts(virt_addr_from_size_t(start_va));
    PageTableEntry* pte = find_pte(&pt , vpn);
    
    //拿到物理页地址
    // int mask = ~( (1 << 10) -1 );
    // u64 phyaddr = ( pte->bits & mask) << 2 ;
    u64 phyaddr = PTE2PA(pte->bits);
    //拿到偏移地址
    u64 page_offset = start_va & 0xFFF;
    //得到物理地址
    u64 data_d = phyaddr + page_offset;
    return (char*) data_d;
}

void __sys_write(size_t fd, const char* data, size_t len)
{

    if(fd == stdout || fd == stderr)
    {
        //printk(" ");
        /*不在需要软件转换，交给mmu*/
        // char* str = translated_byte_buffer(data);
        printk("%s",data);
    }
    else
    {
        panic("Unsupported fd in sys_write!");
    }
}

void __sys_read(size_t fd,const char* data,size_t len)
{
    if(fd == stdin)
    {
        int c;
        assert(len == 1);
        while(1)
        {
            c = sbi_console_getchar();
            if(c!= -1)
                break;
            //读不到数据就切换进程，这样就不会阻塞在这里
            schedule();
            continue;
        }
        char *str = translated_byte_buffer(data);
        str[0] = c;
    }
}

void __sys_yield()
{
    schedule();
}

uint64_t __sys_gettime()
{
    return get_time_us();
}

int __sys_exec(const char *name)
{
    char *app_name = translated_byte_buffer(name);
    printk("exec app_name:%s\n",app_name);
    return exec(app_name);
}

void __sys_exit(u64 exit_code)
{
    exit_current_and_run_next(exit_code);
}

int __sys_wait()
{
    return wait();
}
uint64_t __SYSCALL(size_t syscall_id, reg_t arg1, reg_t arg2, reg_t arg3) {
        switch (syscall_id)
        {
        
        case __NR_write:
            __sys_write(arg1, arg2, arg3);
            break;
        case __NR_read:
            __sys_read(arg1, arg2, arg3);
        case __NR_sched_yield:
            __sys_yield();
            break;
        case __NR_exit:
            __sys_exit(arg1);
            break;
        case __NR_gettimeofday:
            return __sys_gettime();
        case __NR_clone:
            return __sys_fork();
        case __NR_execve:
            return __sys_exec(arg2);
        case __NR_waitid:
            return __sys_wait();
        default:
            printk("Unsupported syscall id:%d\n",syscall_id);
            break;
        }
}

