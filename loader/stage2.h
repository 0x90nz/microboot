#pragma once

struct startup_info {
    uint16_t extended1;
    uint16_t extended2;
    uint16_t configured1;
    uint16_t configured2;
    uint16_t drive_number;
} __attribute__((packed));


#define BSCODE      __attribute__ ((section("bootstrap")))
