#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    OP_NOP     = 0x00,
    OP_MOV     = 0x01,
    OP_MATMUL  = 0x02, // MATMUL dest, src1, src2
    OP_VADD    = 0x03, // VADD dest, src1, src2
    OP_HALT    = 0xFF
} Opcode;

typedef struct {
    uint32_t flags;       // e.g., Quantization format
    uint16_t rows;        
    uint16_t cols;        
    uint32_t stride;      
    float scale;          // Dequantization block scale
    int8_t* data_ptr;     // Direct pointer into the flat memory arena
} TensorReg;

typedef struct {
    uint32_t core_id;
    int8_t* memory_arena;
    TensorReg registers[16];
    const uint32_t* pc;   // Program counter (bytecode stream)
    bool running;
} VMWorkerContext;

// High-speed inner execution loop
void vm_worker_loop(VMWorkerContext* ctx) {
    ctx->running = true;

    while (ctx->running) {
        // Fetch 32-bit fixed-width instruction
        uint32_t instr = *(ctx->pc++);
        
        // Decode fields: [Opcode:8][Dest:4][Src1:4][Src2:4][Reserved/Imm:12]
        uint8_t op   = (instr >> 24) & 0xFF;
        uint8_t dest = (instr >> 20) & 0x0F;
        uint8_t src1 = (instr >> 16) & 0x0F;
        uint8_t src2 = (instr >> 12) & 0x0F;

        switch (op) {
            case OP_NOP:
                break;

            case OP_MOV:
                ctx->registers[dest] = ctx->registers[src1];
                break;

            case OP_MATMUL:
                // Macro-math execution block
                exec_matmul(&ctx->registers[dest], 
                            &ctx->registers[src1], 
                            &ctx->registers[src2]);
                break;

            case OP_VADD:
                exec_vadd(&ctx->registers[dest], 
                          &ctx->registers[src1], 
                          &ctx->registers[src2]);
                break;

            case OP_HALT:
                ctx->running = false;
                break;

            default:
                // Invalid opcode trap
                ctx->running = false;
                break;
        }
    }
}