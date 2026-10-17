#include <string.h>

#include "vm.h"
#include "tile.h"

void flux9_init(Flux9VM *vm) {
    int r, i, j;

    for (r = 0; r < REG_COUNT; r++) {
        for (i = 0; i < T_SIZE; i++) {
            for (j = 0; j < T_SIZE; j++) {
                vm->registers[r].data[i][j] = 0;
            }
        }
    }
    vm->memory = 0;
    vm->pc = 0;
    vm->sp = 0;
    for (i = 0; i < 256; i++) {
        vm->stack[i] = 0;
    }
}

void flux9_run(Flux9VM *vm) {
    for (;;) {
        uint32_t instr;
        uint32_t imm;
        uint8_t op, d, a, b;

        if (vm->memory == 0) {
            return;
        }
        if (vm->pc + 4u > ARENA_SIZE) {
            return;
        }

        memcpy(&instr, vm->memory + vm->pc, sizeof(instr));
        vm->pc += 4;

        op  = (uint8_t)((instr >> 24) & 0xFFu);
        d   = (uint8_t)((instr >> 20) & 0x0Fu);
        a   = (uint8_t)((instr >> 16) & 0x0Fu);
        b   = (uint8_t)((instr >> 12) & 0x0Fu);
        imm = instr & 0x0FFFu;

        switch (op) {
        case OP_HALT:
            return;

        case OP_MOV:
            vm->registers[d] = vm->registers[a];
            break;

        case OP_MATMUL:
            matmul_4x4(&vm->registers[d],
                       &vm->registers[a],
                       &vm->registers[b]);
            break;

        case OP_VADD:
            vadd_4x4(&vm->registers[d],
                     &vm->registers[a],
                     &vm->registers[b]);
            break;

        case OP_LD:
            /* Td ← memory[imm] (one utensor_t) */
            if (imm + sizeof(utensor_t) > ARENA_SIZE) {
                return;
            }
            memcpy(&vm->registers[d], vm->memory + imm, sizeof(utensor_t));
            break;

        case OP_ST:
            /* memory[imm] ← Ta */
            if (imm + sizeof(utensor_t) > ARENA_SIZE) {
                return;
            }
            memcpy(vm->memory + imm, &vm->registers[a], sizeof(utensor_t));
            break;

        default:
            /* Unknown opcode: halt */
            return;
        }
    }
}
