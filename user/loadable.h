#pragma once

#include <stdint.h>

#define START_SECT      __attribute__((section("startinfo")))
extern uint32_t LOADABLE_SIZE;