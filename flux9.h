/* flux9.h - Core Virtual Hardware Definition */
#ifndef FLUX9_H
#define FLUX9_H

#include <stdint.h>

#define T_SIZE 4
#define REG_COUNT 16
#define ARENA_SIZE 65536u  /* flat memory arena (bytes) */

/* Instruction word:
 *   [31:24] opcode  [23:20] dest  [19:16] srcA  [15:12] srcB  [11:0] imm
 * LD/ST: imm is a byte offset (12-bit, first 4 KiB of the arena).
 */
#define OP_HALT   0x00
#define OP_MOV    0x01
#define OP_MATMUL 0x02
#define OP_VADD   0x03
#define OP_LD     0x04
#define OP_ST     0x05

#define ENC(op, d, a, b) \
    (((uint32_t)(op) << 24) | ((uint32_t)(d) << 20) | \
     ((uint32_t)(a) << 16) | ((uint32_t)(b) << 12))

#define ENC_IMM(op, d, a, imm) \
    (((uint32_t)(op) << 24) | ((uint32_t)(d) << 20) | \
     ((uint32_t)(a) << 16) | ((uint32_t)(imm) & 0x0FFFu))

/* The fundamental Micro-Tensor Primitives */
typedef struct {
    int16_t data[T_SIZE][T_SIZE];
} utensor_t;

/* The flux9 Central Processing State */
typedef struct {
    utensor_t registers[REG_COUNT]; /* 16 matrix registers (T0 - T15) */
    uint8_t   *memory;              /* Flat software-defined RAM arena */
    uint32_t  pc;                   /* Program Counter (byte offset) */
    uint32_t  sp;                   /* Scalar/Address Stack Pointer */
    uint32_t  stack[256];           /* Control flow / addressing stack */
} Flux9VM;

#endif /* FLUX9_H */
