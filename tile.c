#include "tile.h"

void matmul_4x4(utensor_t *dest, const utensor_t *a, const utensor_t *b) {
    int i, j, k;

    for (i = 0; i < T_SIZE; i++) {
        for (j = 0; j < T_SIZE; j++) {
            int32_t acc = 0;
            for (k = 0; k < T_SIZE; k++) {
                acc += (int32_t)a->data[i][k] * (int32_t)b->data[k][j];
            }
            dest->data[i][j] = (int16_t)acc;
        }
    }
}

void vadd_4x4(utensor_t *dest, const utensor_t *a, const utensor_t *b) {
    int i, j;

    for (i = 0; i < T_SIZE; i++) {
        for (j = 0; j < T_SIZE; j++) {
            dest->data[i][j] = (int16_t)(
                (int32_t)a->data[i][j] + (int32_t)b->data[i][j]);
        }
    }
}
