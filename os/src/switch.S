# 宏定义 SAVE_SN 参数:n 将s\n的内容保存到(\n+2)*8(a0)
# 目的是前2*8的位置保存了sp和ra
.altmacro
.macro SAVE_SN n
    sd s\n, (\n+2)*8(a0)
.endm

.macro LOAD_SN n
    ld s\n, (\n+2)*8(a1)
.endm
    .section .text
    .globl __switch
__switch:
    
    # save kernel stack of current task,a0存放的是当下任务上下文结构体地址
    sd sp, 8(a0)
    # save ra & s0~s11 of current execution
    sd ra, 0(a0)
    # 设置n为0，重复循环12次宏SAVE_SN，随后设置n+1，目的是保存s0~s11寄存器
    .set n, 0
    .rept 12
        SAVE_SN %n
        .set n, n + 1
    .endr
    # restore ra & s0~s11 of next execution
    ld ra, 0(a1)
    .set n, 0
    .rept 12
        LOAD_SN %n
        .set n, n + 1
    .endr
    # restore kernel stack of next task
    ld sp, 8(a1)
    # 返回到ra寄存器的存储的地址，第一次调用返回到__restore
    ret