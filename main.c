/* Phase 4: arena + LD/ST; MATMUL via T3/T4. stack[] stays untouched. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(void) {
    Flux9VM vm;
    uint8_t *arena;
    utensor_t expect;
    int i, j;
    int ok = 1;

    arena = (uint8_t *)malloc(ARENA_SIZE);
    if (arena == NULL) {
        printf("phase 4 failed: malloc\n");
        return 1;
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
    print_tensor("T3 (from arena)", &vm.registers[3]);
    print_tensor("T4 (from arena)", &vm.registers[4]);

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
        return 0;
    }
    printf("phase 4 failed\n");
    return 1;
}
