/* tile.h - Fixed 4x4 register kernels (no bytecode) */
#ifndef TILE_H
#define TILE_H

#include "flux9.h"

/* dest = a * b. Dot products accumulate in int32_t.
 * Elements should stay in -64..64 so a 4-wide sum fits in int32. */
void matmul_4x4(utensor_t *dest, const utensor_t *a, const utensor_t *b);

/* dest = a + b, elementwise. */
void vadd_4x4(utensor_t *dest, const utensor_t *a, const utensor_t *b);

#endif /* TILE_H */
