#include "os.h"
#define CLOCK_FREQ 10000000
#define TICKS_PER_SEC 500
/*设置下次时钟中断的cnt值*/
/*CLOCK_FREQ为每秒时钟周期，TICKS_PER_SEC为每秒中断次数*/
void set_next_trigger()
{
    sbi_set_timer(r_mtime() + CLOCK_FREQ/TICKS_PER_SEC);
}

/*开启S模式下的时钟中断，目的是s态的时钟中断来让u态的应用程序Trap*/
void timer_init()
{
    reg_t sstatus = r_sstatus();
    sstatus |= (1L << 1);//sstatus的sie置1，设置S态的中断,此处就算不设为1，u态依旧会被s态中断，s态优先级更高
    w_sstatus(sstatus);
    reg_t sie = r_sie();
    sie |= SIE_STIE;//SIE的stie置1，设置时钟中断
    w_sie(sie);
    set_next_trigger();
}

/* 以us为单位返回时间 */
uint64_t get_time_us()
{
    /*CLOCK_FREQ / TICKS_PER_SEC为两次中断间隔的时钟周期，mtime为时钟周期数*/
    reg_t time =  r_mtime() / (CLOCK_FREQ / TICKS_PER_SEC);//时钟中断次数?
    return time;
}