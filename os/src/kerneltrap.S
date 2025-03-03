# tampoline入口
.section .text.trampoline
.globl __alltraps
.align 3
__alltraps:
    #sscratch获取s模式下的sp，同时把u模式的sp保存到sscratch寄存器中
    csrrw sp,sscratch,sp
    #此时需要把栈指针移动到低地址处(栈是向低地址扩展的).34个寄存器，每个需要8字节.
    #addi sp,sp,-34*8
    #保存通用寄存器到用户Trap上下文处
    sd x1,1*8(sp)
    # skip sp(x2), we will save it later
    sd x3, 3*8(sp)
    # skip tp(x4), application does not use it
    # save x5~x31
    sd x4, 4*8(sp)
    sd x5, 5*8(sp)
    sd x6, 6*8(sp)
    sd x7, 7*8(sp)
    sd x8, 8*8(sp)
    sd x9, 9*8(sp)
    sd x10,10*8(sp)
    sd x11, 11*8(sp)
    sd x12, 12*8(sp)
    sd x13, 13*8(sp)
    sd x14, 14*8(sp)
    sd x15, 15*8(sp)
    sd x16, 16*8(sp)
    sd x17, 17*8(sp)
    sd x18, 18*8(sp)
    sd x19, 19*8(sp)
    sd x20, 20*8(sp)
    sd x21, 21*8(sp)
    sd x22, 22*8(sp)
    sd x23, 23*8(sp)
    sd x24, 24*8(sp)
    sd x25, 25*8(sp)
    sd x26, 26*8(sp)
    sd x27, 27*8(sp)
    sd x28, 28*8(sp)
    sd x29, 29*8(sp)
    sd x30, 30*8(sp)
    sd x31, 31*8(sp)

    #保存sstatus和sepc寄存器
    csrr t0,sstatus
    csrr t1,sepc
    sd t0,32*8(sp)
    sd t1,33*8(sp)

    #读取sscratch中的用户栈地址，保存到内核栈
    csrr t2,sscratch
    sd t2,2*8(sp)
    #载入内核地址空间的token
    ld t0, 34*8(sp)
    #载入traphandler入口点地址
    ld t1, 36*8(sp)
    #将sp修改为应用内核栈地址
    ld sp, 35*8(sp)
    #切换到内核地址空间
    csrw satp, t0
    #清空快表
    sfence.vma
    #跳到处理函数
    jr t1

.globl __restore
.align 3
__restore:
    
    # case2: back to U after handling trap
    # mv sp, a0
    # now sp->kernel stack(after allocated), sscratch->user stack
    # restore sstatus/sepc
    #切换到用户空间
    csrw satp, a1
    sfence.vma
    #将传入的Trap上下文位置保存在sscratch
    csrw sscratch, a0
    #sp指向Trap上下文位置(应用地址空间)
    mv sp, a0
    ld t0, 32*8(sp)
    ld t1, 33*8(sp)
    csrw sstatus, t0
    # 恢复sepc寄存器，指向app入口
    csrw sepc, t1
    # restore general-purpuse registers except sp/tp
    ld x1, 1*8(sp)
    ld x3, 3*8(sp)
    ld x4, 4*8(sp)
    ld x5, 5*8(sp)
    ld x6, 6*8(sp)
    ld x7, 7*8(sp)
    ld x8, 8*8(sp)
    ld x9, 9*8(sp)
    ld x10,10*8(sp)
    ld x11, 11*8(sp)
    ld x12, 12*8(sp)
    ld x13, 13*8(sp)
    ld x14, 14*8(sp)
    ld x15, 15*8(sp)
    ld x16, 16*8(sp)
    ld x17, 17*8(sp)
    ld x18, 18*8(sp)
    ld x19, 19*8(sp)
    ld x20, 20*8(sp)
    ld x21, 21*8(sp)
    ld x22, 22*8(sp)
    ld x23, 23*8(sp)
    ld x24, 24*8(sp)
    ld x25, 25*8(sp)
    ld x26, 26*8(sp)
    ld x27, 27*8(sp)
    ld x28, 28*8(sp)
    ld x29, 29*8(sp)
    ld x30, 30*8(sp)
    ld x31, 31*8(sp)

    #保存Trap上下文栈顶位置
    ld sp, 2*8(sp)
    sret