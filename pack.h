/* pack.h - Slice 4x4 tiles out of / into row-major int16 matrices */
#ifndef PACK_H
#define PACK_H

#include "flux9.h"

/* Copy tile from M[row0..row0+3][col0..col0+3]. stride = cols of M. */
void fill_tile(utensor_t *tile, const int16_t *M,
               int row0, int col0, int stride);

/* Write tile into M[row0..row0+3][col0..col0+3]. stride = cols of M. */
void write_tile(int16_t *M, const utensor_t *tile,
                int row0, int col0, int stride);

#endif /* PACK_H */
