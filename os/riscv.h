#ifndef __RISCV_H__
#define __RISCV_H__

#include "os.h"

static inline reg_t r_sepc()
{
    reg_t x;
    asm volatile("csrr %0,sepc": "=r" (x));
    return x;
}

/* scause 记录了异常原因 */
static inline reg_t r_scause()
{
    reg_t x;
    asm volatile("csrr %0, scause" : "=r" (x) );
    return x;
}

// stval 记录了trap发生时的地址
static inline reg_t r_stval()
{
    reg_t x;
    asm volatile("csrr %0, stval" : "=r" (x) );
    return x;
}

/* sstatus记录S模式下处理器内核的运行状态*/
static inline reg_t r_sstatus()
{
    reg_t x;
    asm volatile("csrr %0, sstatus" : "=r" (x) );
    return x;
}


static inline void  w_sstatus(reg_t x)
{
    asm volatile("csrw sstatus, %0" : : "r" (x));
}

/* stvec寄存器 */
static inline void  w_stvec(reg_t x)
{
    asm volatile("csrw stvec, %0" : : "r" (x));
}

static inline reg_t r_stvec()
{
    reg_t x;
    asm volatile("csrr %0, stvec" : "=r" (x) );
    return x;
}

#define SIE_SEIE (1L << 9)
#define SIE_STIE (1L << 5)
#define SIE_SSIE (1L << 1)

/*volatile告诉编译器不要对这段代码优化,"=r"表示将结果存储到通用寄存器，
(x)表示将通用寄存器的值赋给变量x*/
static inline reg_t r_sie()
{
    reg_t x;
    asm volatile("csrr %0,sie": "=r" (x));
    return x;
}

static inline void w_sie(reg_t x)
{
  asm volatile("csrw sie, %0" : : "r" (x));
}
/*rdtime是opensbi提供的伪指令，读取mtime的值，在opensbi源码中找到*/
static inline reg_t r_mtime()
{
  reg_t x;
  asm volatile("rdtime %0" : "=r"(x));
  // asm volatime("csrr %0, 0x0C01" : "=r" (x) )
  return x;
}


#endif