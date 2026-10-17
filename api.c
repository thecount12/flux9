#include "api.h"
#include "pack.h"
#include "tile.h"
#include "vm.h"

int flux9_matmul(utensor_t *dest, const utensor_t *a, const utensor_t *b) {
    Flux9VM vm;
    uint32_t code[] = {
        ENC(OP_MATMUL, 0, 1, 2),
        ENC(OP_HALT, 0, 0, 0),
    };

    flux9_init(&vm);
    vm.registers[1] = *a;
    vm.registers[2] = *b;
    vm.memory = (uint8_t *)code;
    vm.pc = 0;
    flux9_run(&vm);
    *dest = vm.registers[0];
    return 0;
}

static int16_t sat_i16(int32_t v) {
    if (v > 32767) {
        return 32767;
    }
    if (v < -32768) {
        return -32768;
    }
    return (int16_t)v;
}

int flux9_matmul_tiled(int16_t *C, const int16_t *A, const int16_t *B,
                       int M, int N, int K) {
    int i, j, k, r, c;

    if (C == 0 || A == 0 || B == 0) {
        return -1;
    }
    if (M <= 0 || N <= 0 || K <= 0) {
        return -1;
    }
    if ((M % T_SIZE) != 0 || (N % T_SIZE) != 0 || (K % T_SIZE) != 0) {
        return -1;
    }

    for (i = 0; i < M; i += T_SIZE) {
        for (j = 0; j < N; j += T_SIZE) {
            int32_t acc[T_SIZE][T_SIZE];
            utensor_t out;

            for (r = 0; r < T_SIZE; r++) {
                for (c = 0; c < T_SIZE; c++) {
                    acc[r][c] = 0;
                }
            }

            for (k = 0; k < K; k += T_SIZE) {
                utensor_t tileA, tileB, partial;

                fill_tile(&tileA, A, i, k, K);
                fill_tile(&tileB, B, k, j, N);
                /* Same kernel OP_MATMUL uses — no VM re-entry per tile. */
                matmul_4x4(&partial, &tileA, &tileB);

                for (r = 0; r < T_SIZE; r++) {
                    for (c = 0; c < T_SIZE; c++) {
                        acc[r][c] += (int32_t)partial.data[r][c];
                    }
                }
            }

            for (r = 0; r < T_SIZE; r++) {
                for (c = 0; c < T_SIZE; c++) {
                    out.data[r][c] = sat_i16(acc[r][c]);
                }
            }
            write_tile(C, &out, i, j, N);
        }
    }
    return 0;
}
