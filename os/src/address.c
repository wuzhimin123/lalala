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
extern char kernelend[];

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

/*定义页表*/
typedef struct
{
    PhysPageNum root_ppn;//根节点物理页号
    Stack frames;       //保存页表所有节点所在的页帧（页表号）
}PageTable;

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

//根据虚拟地址和一级页表索引到三级页表的页表项
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

//将物理页号与虚拟地址索引的三级页表中的页表项映射起来
void PageTable_map(PageTable* pt,VirtPageNum vpn,PhysPageNum ppn,uint8_t pteflgs)
{
    
    PageTableEntry* pte = find_pte_create(pt,vpn);
    assert(!PageTableEntry_is_valid(pte));
    *pte = PageTableEntry_new(ppn,PTE_V|pteflgs);
}

//取消映射
void PageTable_unmap(PageTable* pt,VirtPageNum vpn)
{
    PageTableEntry* pte = find_pte(pt,vpn);
    assert(!PageTableEntry_is_valid(pte));
    //设为空
    *pte = PageTableEntry_empty();
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