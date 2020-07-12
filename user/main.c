#include <stdlib.h>
#include <stdint.h>
#include <interrupts.h>

int ticks;

void handle_irq0(uint32_t int_no, uint32_t err_no)
{
    ticks++;
}

void main()
{
    ticks = 0;
    register_handler(IRQ_TO_INTR(0), handle_irq0);

    char temp[64];

    while (1)
    {
        puts(">>> ");
        gets(temp);
        
        if (*temp)
        {
            if (strcmp(temp, "ticks") == 0)
            {
                itoa(ticks, temp, 10);
                puts(temp);
                puts("\n");
            }
        }
    }
}