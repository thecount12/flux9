/* prog.h - Program + matrices for the flux9 arena */
#ifndef PROG_H
#define PROG_H

#include "flux9.h"

/* Tile slots in the arena (byte offsets; after the code image). */
#define PROG_SLOT_A 64
#define PROG_SLOT_B 96

/* Input matrices for T1 and T2 (values in -64..64). */
static const int16_t prog_t1[T_SIZE][T_SIZE] = {
    { 1,  2,  3,  4},
    { 5,  6,  7,  8},
    { 9, 10, 11, 12},
    {13, 14, 15, 16},
};

static const int16_t prog_t2[T_SIZE][T_SIZE] = {
    { 2, -1,  0,  1},
    { 0,  3, -2,  1},
    { 1,  0,  2, -1},
    {-1,  2,  0,  3},
};

/* Known product T1 * T2 — the exit-test oracle. */
static const int16_t prog_expect[T_SIZE][T_SIZE] = {
    { 1, 13,  2, 12},
    { 9, 29,  2, 28},
    {17, 45,  2, 44},
    {25, 61,  2, 60},
};

/*
 * ST T1 → slot A
 * ST T2 → slot B
 * LD T3 ← slot A
 * LD T4 ← slot B
 * MATMUL T0, T3, T4
 * HALT
 */
static uint32_t prog_code[] = {
    ENC_IMM(OP_ST, 0, 1, PROG_SLOT_A),
    ENC_IMM(OP_ST, 0, 2, PROG_SLOT_B),
    ENC_IMM(OP_LD, 3, 0, PROG_SLOT_A),
    ENC_IMM(OP_LD, 4, 0, PROG_SLOT_B),
    ENC(OP_MATMUL, 0, 3, 4),
    ENC(OP_HALT, 0, 0, 0),
};

#define PROG_CODE_BYTES (sizeof(prog_code))

#endif /* PROG_H */
