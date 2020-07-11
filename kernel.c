void hang() { while (1) { asm("hlt"); } }

int kernel_main()
{
    *((unsigned char*)0xb8000) = 'A';

    hang();
}