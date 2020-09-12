int foo()
{
    return 100;
}

void main()
{
    for (int i = 0; i < foo(); i++) { asm("hlt"); }
}