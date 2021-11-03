#include <stdint.h>
#include "backtrace.h"
#include "printf.h"
#include "stdlib.h"
#include "io/ansi_colours.h"

void backtrace()
{
	printf("\nbacktrace:\n");
    dprintf_raw(ANSI_COLOR_RED "\nbacktrace: ");

	void** frame;
	for (frame = __builtin_frame_address(1);
			(uint32_t)frame >= 0x1000 && frame[0] != NULL;
			frame = frame[0]) {
		printf(" %p\n", frame[1]);
        dprintf_raw("0x%p ");
	}

    dprintf_raw(ANSI_COLOR_RESET);
}

