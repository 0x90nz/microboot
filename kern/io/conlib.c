#include "conlib.h"
#include <stddef.h>
#include "chardev.h"
#include "../config.h"

void cl_repeat(char c, int n)
{
    chardev_t* output = *(chardev_t**)config_getobj("sys:&stdout");
    while (n > 0) {
        output->putc(output, c);
        n--;
    }
}

void cl_box(char corner, char v_edge, char h_edge, int width, const char* content, int flags)
{
    // the number of chars available for text output (we keep one padding on either side,
    // and one box char on each side as well)
    const int nr_textchr = width - 4;
    chardev_t* output = *(chardev_t**)config_getobj("sys:&stdout");

    output->putc(output, corner);
    cl_repeat(h_edge, nr_textchr + 2);
    output->putc(output, corner);
    output->putc(output, '\n');

    while (*content) {
        int left = nr_textchr;

        // border
        output->putc(output, v_edge);
        output->putc(output, ' ');

        // main text
        while (left > 0 && *content) {
            // if this is a newline we eat it and just display a blank rest of the line.
            if (*content == '\n') {
                content++;
                break;
            }

            output->putc(output, *content);
            content++;
            left--;
        }

        // any padding space needed to complete line
        while (left > 0) {
            output->putc(output, ' ');
            left--;
        }

        // border
        output->putc(output, ' ');
        output->putc(output, v_edge);
        output->putc(output, '\n');
    }


    output->putc(output, corner);
    cl_repeat(h_edge, nr_textchr + 2);
    output->putc(output, corner);
    output->putc(output, '\n');
}

