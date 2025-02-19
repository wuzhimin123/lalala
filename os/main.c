extern sbi_console_putchar(int ch);

void os_main()
{
    for(int i = 0;i<1;i++)
    {
        sbi_console_putchar('L');
        sbi_console_putchar('o');
        sbi_console_putchar('v');
        sbi_console_putchar('e');
        sbi_console_putchar('Z');
        sbi_console_putchar('h');
        sbi_console_putchar('a');
        sbi_console_putchar('n');
        sbi_console_putchar('g');
        sbi_console_putchar('Y');
        sbi_console_putchar('e');
    }
}