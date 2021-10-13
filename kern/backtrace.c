#include <stdint.h>
#include "backtrace.h"
#include "printf.h"

void backtrace()
{
	printf("\nbacktrace:\n");

	void** frame;
	for (frame = __builtin_frame_address(1);
			(uint32_t)frame >= 0x1000 && frame[0] != NULL;
			frame = frame[0]) {
		printf(" %p\n", frame[1]);
		debugf("%p", frame[1]);
	}
}

