#include <stdlib.h>

void main()
{
    char temp[64];

    while (1)
    {
        puts(">>> ");
        gets(temp);
        
        if (*temp)
        {
            puts(temp);
            puts("\n");
        }
    }
}