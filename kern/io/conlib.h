#pragma once

#include "chardev.h"

enum conlib_flags {
    CL_NONE         = 0,
};

/**
 * @brief outputs a box of specified with with the given message and edge
 * characters
 *
 * If the content contains a newline, it will be treated as a newline for the
 * content /within/ the box (and will still print the ending character
 * correctly). If the input exceeds the space available in the box, it will
 * automatically be wrapped, but not in a particularly graceful manner. Spaces
 * will be preserved, and no hypenation occurs, so it is advisable to pre-wrap
 * text if such an effect is required.
 *
 * @param corner the character to use for the corners of the box
 * @param v_edge the character to use for the vertical edges of the box
 * @param h_edge the character to use for the horizontal edges of the box
 * @param width the width of the box to create (in characters)
 * @param content the message to display within the box
 * @param flags flags altering the display of this box
 */
void cl_box(char corner, char v_edge, char h_edge, int width, const char* content, int flags);

/**
 * @brief Output a character a given number of times
 *
 * @param c the character to output
 * @param n the number of times to repeat the character
 */
void cl_repeat(char c, int n);

