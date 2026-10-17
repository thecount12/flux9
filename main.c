/* Phase 4 arena demo + Phase 6 tiled matmul exit test. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "prog.h"
#include "vm.h"

static void print_tensor(const char *label, const utensor_t *t) {
    int i, j;
    printf("--- %s ---\n", label);
    for (i = 0; i < T_SIZE; i++) {
        for (j = 0; j < T_SIZE; j++) {
            printf("%6d", (int)t->data[i][j]);
        }
        printf("\n");
    }
}

static int stack_untouched(const Flux9VM *vm) {
    int i;

    if (vm->sp != 0) {
        return 0;
    }
    for (i = 0; i < 256; i++) {
        if (vm->stack[i] != 0) {
            return 0;
        }
    }
    return 1;
}

static int run_phase4(void) {
    Flux9VM vm;
    uint8_t *arena;
    utensor_t expect;
    int i, j;
    int ok = 1;

    arena = (uint8_t *)malloc(ARENA_SIZE);
    if (arena == NULL) {
        printf("phase 4 failed: malloc\n");
        return 0;
    }
    memset(arena, 0, ARENA_SIZE);
    memcpy(arena, prog_code, PROG_CODE_BYTES);

    flux9_init(&vm);
    vm.memory = arena;
    vm.pc = 0;

    memcpy(vm.registers[1].data, prog_t1, sizeof(prog_t1));
    memcpy(vm.registers[2].data, prog_t2, sizeof(prog_t2));
    memcpy(expect.data, prog_expect, sizeof(prog_expect));

    flux9_run(&vm);

    print_tensor("T0", &vm.registers[0]);

    if (memcmp(vm.registers[3].data, prog_t1, sizeof(prog_t1)) != 0 ||
        memcmp(vm.registers[4].data, prog_t2, sizeof(prog_t2)) != 0) {
        printf("FAIL: LD did not restore T1/T2 into T3/T4\n");
        ok = 0;
    }

    for (i = 0; i < T_SIZE; i++) {
        for (j = 0; j < T_SIZE; j++) {
            if (vm.registers[0].data[i][j] != expect.data[i][j]) {
                printf("FAIL: T0 != expect\n");
                ok = 0;
                goto done;
            }
        }
    }

    if (!stack_untouched(&vm)) {
        printf("FAIL: stack was touched\n");
        ok = 0;
    }

done:
    free(arena);
    if (ok) {
        printf("phase 4 passed\n");
    } else {
        printf("phase 4 failed\n");
    }
    return ok;
}

/* Naive C = A * B for int16 row-major buffers (reference). */
static void ref_matmul(int16_t *C, const int16_t *A, const int16_t *B,
                       int M, int N, int K) {
    int i, j, k;
    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            int32_t acc = 0;
            for (k = 0; k < K; k++) {
                acc += (int32_t)A[i * K + k] * (int32_t)B[k * N + j];
            }
            if (acc > 32767) {
                acc = 32767;
            } else if (acc < -32768) {
                acc = -32768;
            }
            C[i * N + j] = (int16_t)acc;
        }
    }
}

static int run_phase6(void) {
    enum { M = 8, N = 8, K = 8 };
    int16_t A[M * K];
    int16_t B[K * N];
    int16_t C[M * N];
    int16_t expect[M * N];
    int i, j;
    int ok = 1;

    /* Small values so 8-wide int16 products stay in int16 after sat. */
    for (i = 0; i < M; i++) {
        for (j = 0; j < K; j++) {
            A[i * K + j] = (int16_t)((i + j) % 5 - 2);
        }
    }
    for (i = 0; i < K; i++) {
        for (j = 0; j < N; j++) {
            B[i * N + j] = (int16_t)((i * 2 + j) % 5 - 2);
        }
    }

    if (flux9_matmul_tiled(C, A, B, M, N, K) != 0) {
        printf("FAIL: flux9_matmul_tiled rejected good dims\n");
        return 0;
    }
    ref_matmul(expect, A, B, M, N, K);

    for (i = 0; i < M * N; i++) {
        if (C[i] != expect[i]) {
            printf("FAIL: tiled C[%d]=%d expect=%d\n",
                   i, (int)C[i], (int)expect[i]);
            ok = 0;
            break;
        }
    }

    /* Non-multiple of 4 must fail. */
    if (flux9_matmul_tiled(C, A, B, 6, 8, 8) != -1) {
        printf("FAIL: expected -1 for non-multiple dims\n");
        ok = 0;
    }

    /* 4x4 tiled path must match single-tile product of prog matrices. */
    {
        int16_t a4[16], b4[16], c4[16];
        int r, c;

        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++) {
                a4[r * 4 + c] = prog_t1[r][c];
                b4[r * 4 + c] = prog_t2[r][c];
            }
        }
        if (flux9_matmul_tiled(c4, a4, b4, 4, 4, 4) != 0) {
            printf("FAIL: 4x4 tiled\n");
            ok = 0;
        } else {
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++) {
                    if (c4[r * 4 + c] != prog_expect[r][c]) {
                        printf("FAIL: 4x4 tiled != prog_expect\n");
                        ok = 0;
                        goto done6;
                    }
                }
            }
        }
    }

done6:
    if (ok) {
        printf("phase 6 passed\n");
    } else {
        printf("phase 6 failed\n");
    }
    return ok;
}

int main(void) {
    int ok4 = run_phase4();
    int ok6 = run_phase6();

    if (ok4 && ok6) {
        return 0;
    }
    return 1;
}
