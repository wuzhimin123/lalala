#include <timeros/address.h>
extern char etext[];
extern char kernelend[];

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

/* 虚拟地址向下取整 */
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

/*新建一个页表项PTE*/
PageTableEntry PageTableEntry_new(PhysPageNum ppn,uint8_t PTEFlags)
{
    PageTableEntry entry;
    entry.bits = (ppn.value << 10)|PTEFlags;
    return entry;
}

/*空页表*/
PageTableEntry PageTableEntry_empty()
{
    PageTableEntry entry;
    entry.bits = 0;
    return entry;
}

/*获取下级页表的物理页号*/
PhysPageNum PageTableEntry_ppn(PageTableEntry *entry)
{
    PhysPageNum ppn;
    ppn.value = (entry->bits >> 10) & ((1ul << 44) - 1);
    return ppn;
}

/*获取页表项标志位，后8位是标志位*/
uint8_t PageTableEntry_flags(PageTableEntry *entry)
{
    return entry->bits & 0xFF;
}

/*判断页表项是否有效*/
bool PageTableEntry_is_valid(PageTableEntry *entry)
{
    uint8_t entryFlags = PageTableEntry_flags(entry);
    return (entryFlags & PTE_V)!=0;
}

/*得到指向字节的指针，用于操作某个物理页的字节单元*/
uint8_t *get_bytes_arry(PhysPageNum ppn)
{
    /*物理页号转换位物理地址*/
    PhysAddr addr = phys_addr_from_phys_page_num(ppn);
    return (uint8_t*)addr.value;
}

/*得到指向PTE的指针，操作存储PTE的物理页的PTE单元*/
PageTableEntry* get_pte_array(PhysPageNum ppn)
{
    PhysAddr addr = phys_addr_from_phys_page_num(ppn);
    return (PageTableEntry*)addr.value;
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

StackFrameAllocator FrameAllocatorImpl;


/*内存分配器初始化，空闲页表从0x80250000开始*/
void frame_alloctor_init()
{
    // 初始化时 kernelend 需向上取整,因为可能起始地址不是一个完整的页的开头，内存管理混乱
    StackFrameAllocator_new(&FrameAllocatorImpl);
    StackFrameAllocator_init(&FrameAllocatorImpl, \
            ceil_phys(phys_addr_from_size_t(kernelend)), \
            ceil_phys(phys_addr_from_size_t(PHYSTOP)));
    printk("Memoery start:%p\n",kernelend);
    printk("Memoery end:%p\n",PHYSTOP);
}



/*获取虚拟页号(虚拟地址的那27位页号)的三级索引*/
void indexes(VirtPageNum vpn,size_t* result)
{
    size_t idx[3];
    for(int i = 2;i >= 0;i--)
    {
        idx[i] = vpn.value & 0x1ff; // 1_1111_1111 = 0x1ff
        vpn.value >>= 9; //右移9位然后赋值给value,用于下一次循环
    }

    for(int i = 0;i < 3;i++)
        result[i] = idx[i];
}

/* 分配一页内存 */
PhysPageNum kalloc(void)
{
    PhysPageNum frame =  StackFrameAllocator_alloc(&FrameAllocatorImpl);
    //printk("frame:%d\n",frame.value);
    return frame;
}

/* 释放一页内存 */
void kfree(PhysPageNum ppn)
{
    StackFrameAllocator_dealloc(&FrameAllocatorImpl,ppn);
}

//根据虚拟地址和一级页表索引到三级页表的页表项(索引不到可自行创建，因此即使根页表为空也可实现映射)
PageTableEntry *find_pte_create(PageTable* pt,VirtPageNum vpn)
{
    //虚拟页号的三级索引保存到idx
    size_t idx[3];
    indexes(vpn,idx);
    //根节点(页号)
    PhysPageNum ppn = pt->root_ppn;
    for(int i = 0;i < 3;i++)
    {
        //从一个页表里根据“虚拟地址的偏移量”得到pte
        //get_pte_array(ppn)得到指向pte的指针，[idx[i]]将其变成数组形式，得到具体的页表项，然后&取地址
        PageTableEntry *pte = &get_pte_array(ppn)[idx[i]];
        //得到三级页表的页表项
        if(i == 2)
            return pte;
        //若页表项为空(无效)
        if(!PageTableEntry_is_valid(pte))
        {
            //分配一页内存，得到页号
            PhysPageNum frame = StackFrameAllocator_alloc(&FrameAllocatorImpl);
            //新建页表项
            *pte = PageTableEntry_new(frame,PTE_V);
            //把页号压入页表pt中
            // push(&pt->frames,frame.value);
        }
        //取出进入下级页表的物理页号
        ppn = PageTableEntry_ppn(pte);
    }

}

//与上面函数类似，但只索引不创建
PageTableEntry* find_pte(PageTable* pt, VirtPageNum vpn)
{
    // 拿到虚拟页号的三级索引，保存到idx数组中
    size_t idx[3];
    indexes(vpn, idx); 
    //根节点
    PhysPageNum ppn = pt->root_ppn;
    //从根节点开始遍历，如果没有pte，就返回空
    for (int i = 0; i < 3; i++) 
    {
        //拿到具体的页表项
        PageTableEntry* pte =  &get_pte_array(ppn)[idx[i]];
        //如果此项页表为空
        if (!PageTableEntry_is_valid(pte)) {
            return NULL;
        }
        if (i == 2) {
                return pte;
            }
        //取出进入下级页表的物理页号
        ppn = PageTableEntry_ppn(pte);
    }
    
}

//将物理页号与虚拟地址索引的三级页表中的页表项映射起来，最终实现va和pa的映射
void PageTable_map(PageTable* pt,VirtAddr va,PhysAddr pa,u64 size,uint8_t pteflgs)
{
    if(size == 0)
        panic("mappages:size");
    //取页表号
    PhysPageNum ppn = floor_phys(pa);
    VirtPageNum vpn = floor_virts(va);
    //取需要分配到哪个页表号，-1有作用，若不减一，va.value为0，size为4096，则last为1，实际上第0页足够分配
    u64 last = (va.value + size - 1)/PAGE_SIZE;
    //一页一页分配
    for(;;)
    {
        PageTableEntry* pte = find_pte_create(pt,vpn);
        assert(!PageTableEntry_is_valid(pte));
        *pte = PageTableEntry_new(ppn,PTE_V|pteflgs);

        if(vpn.value == last)
            break;
        vpn.value += 1;
        ppn.value += 1;
    }
    
}

//取消映射
void PageTable_unmap(PageTable* pt,VirtPageNum vpn)
{
    
    PageTableEntry* pte = find_pte(pt,vpn);
    assert(!PageTableEntry_is_valid(pte));
    //设为空
    *pte = PageTableEntry_empty();
}

/*内核代码段数据段恒等映射，得到映射的根页表*/
PageTable kvmmake(void)
{
    PageTable pt;
    //分配一个空闲页表作为根页表(一级页表)
    PhysPageNum root_ppn = StackFrameAllocator_alloc(&FrameAllocatorImpl);
    pt.root_ppn = root_ppn;
    printk("root_ppn:%p\n",phys_addr_from_phys_page_num(root_ppn));

    printk("etext:%p\n",(u64)etext);
    //实现内核text段恒等映射,可读可执行,u模式不可访问。(etext在链接脚本文件中定义)
    PageTable_map(&pt,virt_addr_from_size_t(KERNBASE),phys_addr_from_size_t(KERNBASE), \
                    (u64)etext - KERNBASE, PTE_R|PTE_X);
    printk("finish kernel text map!\n");
    //实现内核data段恒等映射，可读可写，u模式不可访问
    PageTable_map(&pt,virt_addr_from_size_t((u64)etext),phys_addr_from_size_t((u64)etext), \
                    PHYSTOP - (u64)etext, PTE_R|PTE_W);
    printk("finish kernel data and physical RAM map!\n");
    return pt;
}

/*内核根页表放在0x80250000，根据这个页表可查到映射关系*/
/*建立内核页表*/
PageTable kernel_pagetable;
void kvminit()
{
    kernel_pagetable = kvmmake();
}

/*写satp寄存器，清空快表。快表是MMU根据页表映射关系设置的，MMU会首先读快表，然后读根页表*/
void kvminithart()
{
    // wait for any previous writes to the page table memory to finish.
    //MAKE_SATP，启动SV39模式，并将根页表号写入(得到一个将要写入satp寄存器的值)
    printk("satp:%lx\n",MAKE_SATP(kernel_pagetable.root_ppn.value));
    sfence_vma();//第一次清空TLB是satp切换内容之前有旧的映射
    //写入到satp寄存器
    w_satp(MAKE_SATP(kernel_pagetable.root_ppn.value));
  
    // flush stale entries from the TLB.
    sfence_vma();//第二次清空TLB是第一次清空和satp切换内容之间可能存在页表映射查找,MMU会在快表中记录
    reg_t satp = r_satp();

    printk("satp:%lx\n",satp);

    
}
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