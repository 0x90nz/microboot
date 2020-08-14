#include <stdint.h>

extern uint32_t LOADABLE_SIZE;
__attribute__((section("startinfo"))) uint32_t size = (uint32_t)&LOADABLE_SIZE;

int init()
{
    return 1234;
}