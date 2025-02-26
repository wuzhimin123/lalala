OUTPUT_ARCH(riscv)
ENTRY(_start)
BASE_ADDRESS = 0x80200000;
MEMORY
{ 
	ram (rxai!w) : ORIGIN = 0x80200000, LENGTH = 128M /*!w不可写*/
}
SECTIONS
{
    skernel = .;   /* 定义内核起始内存地址 */
	.text : {		/*代码段*/
		*(.text .text.*)	/*将所有目标文件中的 .text 段和以 .text. 开头的段合并到这里*/
	. = ALIGN(0x1000);
    PROVIDE(etext = .); /*定义一个符号etext，其值为当前地址*/
	} >ram				/*放到ram内存区域*/

	.rodata : {		/*只读数据段*/
		*(.rodata .rodata.*)
	} >ram

	.data : {		/*已初始化数据段，如已初始化全局变量和静态变量*/
		. = ALIGN(4096);
		*(.sdata .sdata.*)
		*(.data .data.*)
		PROVIDE(_data_end = .);
	} >ram

	.bss :{			/*未初始化数据段，如未初始化全局变量和静态变量*/
		*(.sbss .sbss.*)
		*(.bss .bss.*)
		*(COMMON)
	} >ram

	PROVIDE(kernelend = .);
}