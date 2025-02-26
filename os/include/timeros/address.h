#ifndef TOS_ADDRESS_H
#define TOS_ADDRESS_H

#include <timeros/os.h>
#include <timeros/stack.h>
#include <timeros/string.h>
#include <timeros/assert.h>

#define PAGE_SIZE 0x1000    //4kb
#define PAGE_SIZE_BITS 0xc  //12 偏移地址长度
#define PA_WIDTH_SV39 56    //物理地址长度
#define VA_WIDTH_SV39 39    //虚拟地址长度
#define PPN_WIDTH_SV39 (PA_WIDTH_SV39 - PAGE_SIZE_BITS)  // 物理页号 44位 [55:12]
#define VPN_WIDTH_SV39 (VA_WIDTH_SV39 - PAGE_SIZE_BITS)  // 虚拟页号 27位 [38:12]

#define MEMORY_END 0x80800000   //0x80200000-0x80800000
#define MEMORY_START 0x80400000
#define KERNBASE 0x80200000L  
#define PHYSTOP (KERNBASE + 128*1024*1024)

/* 物理地址 */
typedef struct {
    uint64_t value; 
} PhysAddr;

/* 虚拟地址 */
typedef struct {
    uint64_t value;
} VirtAddr;

/* 物理页号 */
typedef struct {
    uint64_t value;
} PhysPageNum;

/* 虚拟页号 */
typedef struct {
    uint64_t value;
} VirtPageNum;

/*定义页表项*/
typedef struct{
    uint64_t bits;
}PageTableEntry;

/*定义页表*/
typedef struct
{
    PhysPageNum root_ppn;//根节点物理页号
}PageTable;

// 定义位掩码常量
#define PTE_V (1 << 0)   //有效位
#define PTE_R (1 << 1)   //可读属性
#define PTE_W (1 << 2)   //可写属性
#define PTE_X (1 << 3)   //可执行属性
#define PTE_U (1 << 4)   //用户访问模式
#define PTE_G (1 << 5)   //全局映射
#define PTE_A (1 << 6)   //访问标志位
#define PTE_D (1 << 7)   //脏位

#define SATP_SV39 (8L << 60) //设置satp寄存器的模式为SV39模式
#define MAKE_SATP(pagetable) (SATP_SV39 | (((u64)pagetable)))

#endif