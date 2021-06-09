#pragma once

#include <stdint.h>

typedef uint32_t pmode_uintptr_t;
typedef uint16_t rmode_uintptr_t;

/**
 * Get the segment of a linear address. Just returns the top 16 bits of the
 * linear address.
 *
 * Must be used in conjunction with OFFOF to be valid.
 */
#define SEGOF(x)        ((pmode_uintptr_t)x >> 4)

/**
 * Get the offset of a linear address. Returns the low 4 bits of the linear
 * address.
 *
 * Must be used with SEGOF to be valid.
 */
#define OFFOF(x)        ((pmode_uintptr_t)x & 0x0f)

/**
 * Convert a linear address to a packed real mode segment:offset pair.
 *
 * The segment:offset pair is encoded in a 32 bit integer where the high word is
 * the segment and the low word is the offset.
 */
#define LIN_TO_REAL(t, lin) (t)(SEGOF(lin) << 16 | OFFOF(lin))

/**
 * Convert a real mode segment:offset pair into a linear address.
 *
 * The calculation to convert is (seg * 0x10) + off.
 */
#define REAL_TO_LIN(t, seg, off) (t)((((pmode_uintptr_t) (seg)) << 4) + ((pmode_uintptr_t)off))

/**
 * Get the segment from a packed segment:offset pair.
 */
#define PACKED_SEGOF(real) (pmode_uintptr_t)(((uint32_t) (real)) >> 16)

/**
 * Get the offset from a packed segment:offset pair.
 */
#define PACKED_OFFOF(real) ((uint32_t) (real)) & 0xffff

/**
 * Convert a packed segment:offset pair to a linear address that can be used in
 * protected mode.
 */
#define U32REAL_TO_LIN(t, real) REAL_TO_LIN(t, PACKED_SEGOF(real), PACKED_OFFOF(real))
