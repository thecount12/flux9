#include "pack.h"

void fill_tile(utensor_t *tile, const int16_t *M,
               int row0, int col0, int stride) {
    int r, c;

    for (r = 0; r < T_SIZE; r++) {
        for (c = 0; c < T_SIZE; c++) {
            tile->data[r][c] = M[(row0 + r) * stride + (col0 + c)];
        }
    }
}

void write_tile(int16_t *M, const utensor_t *tile,
                int row0, int col0, int stride) {
    int r, c;

    for (r = 0; r < T_SIZE; r++) {
        for (c = 0; c < T_SIZE; c++) {
            M[(row0 + r) * stride + (col0 + c)] = tile->data[r][c];
        }
    }
}
