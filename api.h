/* api.h - Entry points for hosts (Lux). */
#ifndef FLUX9_API_H
#define FLUX9_API_H

#include "flux9.h"

/* MATMUL T0,T1,T2 via the bytecode VM. Writes T0 into dest. Returns 0. */
int flux9_matmul(utensor_t *dest, const utensor_t *a, const utensor_t *b);

/*
 * Tiled matmul over flat row-major int16 buffers.
 * A is M×K, B is K×N, C is M×N. All dims must be multiples of T_SIZE (4).
 * Inner products accumulate in int32; results are saturated into int16.
 * Returns 0 on success, -1 on bad dims.
 */
int flux9_matmul_tiled(int16_t *C, const int16_t *A, const int16_t *B,
                       int M, int N, int K);

#endif /* FLUX9_API_H */
