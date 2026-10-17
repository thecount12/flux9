#include "api.h"
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
