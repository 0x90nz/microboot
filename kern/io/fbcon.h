#pragma once

#include "console.h"

enum fbcon_setparam_id {
    // aux = array of 16 colours per console.h (enum console_colour)
    FBCON_SETPARAM_COLOURSCHEME = 1,
    // aux = pointer to fbcon_font
    FBCON_SETPARAM_FONT,
    // aux = NULL
    FBCON_SETPARAM_RESIZE,
};

struct fbcon_font {
    // The data associated with the font, with ' ' (32) being the first character
    // and subsequently following ASCII ordering.
    //
    // Each character is made up of `char_height` rows of pixels, where each row
    // is `char_width` long. The rows are packed into a single byte, where the
    // bits are displayed MSB to LSB. Not all bits must be used, any unused bits
    // are ignored, but it would be wise for them to be zero, so that if they happen
    // to be displayed, it would be blank.
    //
    // The font data within a single row is displayed MSB to LSB for the first
    // byte, then MSB to LSB for the second byte, and so on, until enough bits to
    // reach char_width_bytes have been read.
    //
    // Thus, a single character will take char_width_bytes * char_height bytes.
    uint8_t* data;
    // The width of the font in pixels.
    uint8_t char_width;
    // The number of bytes comprising a single row of data. May be more than required
    // for char_width pixels, for example to aid in alignment.
    uint8_t char_width_bytes;
    // The width of the font in pixels.
    uint8_t char_height;
};

