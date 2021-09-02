#pragma once

#include "console.h"

enum fbcon_setparam_id {
    // aux = array of 16 colours per console.h (enum console_colour)
    FBCON_SETPARAM_COLOURSCHEME = 1,
};

