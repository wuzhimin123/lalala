#include <timeros/address.h>

/*u64转化为物理地址*/
PhysAddr phys_addr_from_size_t(uint64_t v)
{
    PhysAddr addr;
    addr.value = v & ((1ULL << PA_WIDTH_SV39) - 1);    //ULL = undesigned long long 类型
    return addr;
}
/*u64转化为物理页号*/
PhysPageNum phys_page_num_from_size_t(uint64_t v)
{
    PhysPageNum pageNum;
    pageNum.value = v & ((1ULL << PPN_WIDTH_SV39) - 1);    //ULL = undesigned long long 类型
    return pageNum;
}
/*物理地址转为u64*/
uint64_t size_t_from_phys_addr(PhysAddr v) {
    return v.value;
}
/*物理页号转为u64*/
uint64_t size_t_from_phys_page_num(PhysPageNum v) {
    return v.value;
}
/*物理页号转为物理地址*/
PhysAddr phys_addr_from_phys_page_num(PhysPageNum ppn)
{
    PhysAddr addr;
    addr.value = ppn.value << PAGE_SIZE_BITS;
    return addr;
}

/*u64转化为虚拟地址*/
VirtAddr virt_addr_from_size_t(uint64_t v)
{
    VirtAddr addr;
    addr.value = v & ((1ULL << VA_WIDTH_SV39) - 1);
    return addr;
}

/*u64转化为虚拟页号*/
VirtPageNum virt_page_num_from_size_t(uint64_t v)
{
    VirtPageNum pageNum;
    pageNum.value = v & ((1ULL << VPN_WIDTH_SV39) - 1);
    return pageNum;
}

/*虚拟地址转化为u64*/
uint64_t size_t_from_virt_addr(VirtAddr v) {
    if (v.value >= (1ULL << (VA_WIDTH_SV39 - 1))) {
        return v.value | ~((1ULL << VA_WIDTH_SV39) - 1);
    } else {
        return v.value;
    }
}
/*虚拟页号转化为u64*/
uint64_t size_t_from_virt_page_num(VirtPageNum v) {
    return v.value;
}

/* 物理地址向下取整 比如4098/4096=1，那就是物理页1号*/
PhysPageNum floor_phys(PhysAddr phys_addr) {
    PhysPageNum phys_page_num;
    phys_page_num.value = phys_addr.value / PAGE_SIZE;
    return phys_page_num;
}

/* 物理地址向上取整 */
PhysPageNum ceil_phys(PhysAddr phys_addr) {
    PhysPageNum phys_page_num;
    phys_page_num.value = (phys_addr.value + PAGE_SIZE - 1) / PAGE_SIZE;
    return phys_page_num;
}

/* 物理地址向下取整 */
VirtPageNum floor_virts(VirtAddr virt_addr) {
    VirtPageNum virt_page_num;
    virt_page_num.value = virt_addr.value / PAGE_SIZE;
    return virt_page_num;
}

/* 把虚拟地址转换为虚拟页号 */
VirtPageNum virt_page_num_from_virt_addr(VirtAddr virt_addr)
{
    VirtPageNum vpn;
    vpn.value =  virt_addr.value / PAGE_SIZE;
    return vpn;
}
/*内存管理策略核心：栈式物理页帧*/
typedef struct
{
    uint64_t current; //空闲内存的起始物理页号
    uint64_t end;     //空闲内存的结束物理页号
    Stack recycled;   //回收的物理页号
}StackFrameAllocator;
/*创建StackFrameAllocator实例*/
void StackFrameAllocator_new(StackFrameAllocator *allocator)
{
    allocator->current = 0;
    allocator->end = 0;
    initStack(&allocator->recycled);
}
/*初始化为可用物理页号区间*/
void StackFrameAllocator_init(StackFrameAllocator *allocator,PhysPageNum l,PhysPageNum r)
{
    allocator->current = l.value;
    allocator->end = r.value;
}
/*物理页分配*/
PhysPageNum StackFrameAllocator_alloc(StackFrameAllocator *allocator)
{
    PhysPageNum ppn;
    if(allocator->recycled.top >= 0)
        ppn.value = pop(&(allocator->recycled));//优先分配已经回收的
    else
    {
        if(allocator->current == allocator->end)//未使用的物理页分配没了
            ppn.value = 0;
        else
            ppn.value = allocator->current++;   //分配未使用的物理页
    }
    /*清空此页内存*/
    PhysAddr addr = phys_addr_from_phys_page_num(ppn);
    memset(addr.value,0,PAGE_SIZE);
    return ppn;
}

/*物理页回收*/
void StackFrameAllocator_dealloc(StackFrameAllocator *allocator, PhysPageNum ppn)
{
    uint64_t ppnValue = ppn.value;
    //确保ppn不是未分配的页号
    if(ppnValue >= allocator->current)
    {
        printk("Frame ppn = %lx has not been allocated!\n",ppnValue);
        return;
    }
    //确保ppn没有被回收
    if(allocator->recycled.top > 0)
    {
        for(size_t i = 0;i <= allocator->recycled.top;i++)
        {
            if(ppnValue == allocator->recycled.data[i])
            return;
        }
    }
    //回收
    push(&(allocator->recycled),ppnValue);
}

static StackFrameAllocator FrameAllocatorImpl;

void frame_allocator_test()
{
    PhysPageNum frame[10];
    StackFrameAllocator_new(&FrameAllocatorImpl);
    StackFrameAllocator_init(&FrameAllocatorImpl, \
            floor_phys(phys_addr_from_size_t(MEMORY_START)), \
            ceil_phys(phys_addr_from_size_t(MEMORY_END)));
    printk("Memoery start:%d\n",floor_phys(phys_addr_from_size_t(MEMORY_START)));
    printk("Memoery end:%d\n",ceil_phys(phys_addr_from_size_t(MEMORY_END)));
    for (size_t i = 0; i < 5; i++)
    {
         frame[i] = StackFrameAllocator_alloc(&FrameAllocatorImpl);
         printk("frame id:%d\n",frame[i].value);
    }
    for (size_t i = 0; i < 5; i++)
    {
        StackFrameAllocator_dealloc(&FrameAllocatorImpl,frame[i]);
        printk("allocator->recycled.data.value:%d\n",FrameAllocatorImpl.recycled.data[i]);
        printk("frame id:%d\n",frame[i].value);
    }
    PhysPageNum frame_test[10];
    for (size_t i = 0; i < 5; i++)
    {
         frame[i] = StackFrameAllocator_alloc(&FrameAllocatorImpl);
        printk("frame id:%d\n",frame[i].value);
    }
}