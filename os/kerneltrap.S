.globl __alltraps
.align 4
__alltraps:
    #sscratch获取s模式下的sp，同时把u模式的sp保存到sscratch寄存器中
    csrrw sp,sscratch,sp
    #sp在restore中指向了pt_regs结构体的末尾(此结构体已销毁)，此处也就是重新压栈
    #此时需要把栈指针移动到低地址处(栈是向低地址扩展的).34个寄存器，每个需要8字节.
    addi sp,sp,-34*8
    #保存通用寄存器
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

    #把内核栈指针传递给a0，作为参数传递给处理函数
    mv a0,sp
    call trap_handler

.globl __restore
.align 4
__restore:
    # case1: start running app by __restore
    # case2: back to U after handling trap
    #sp指向内核栈中pt_regs结构体起始位置
    mv sp, a0
    # now sp->kernel stack(after allocated), sscratch->user stack
    # restore sstatus/sepc
    ld t0, 32*8(sp)
    ld t1, 33*8(sp)
    ld t2, 2*8(sp)
    csrw sstatus, t0
    csrw sepc, t1
    csrw sscratch, t2
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

    # release TrapContext on kernel stack
    #将sp设为结构体的末尾，代表该结构体释放。
    #释放并不代表内存中的内容被清空，而是可以被别的程序更改
    addi sp, sp, 34*8
    # now sp->kernel stack, sscratch->user stack
    #sp指向用户栈
    csrrw sp, sscratch, sp
    #返回spec中的地址
    sret