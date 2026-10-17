# flux9 plan

flux9 is a 16-register tile machine. This plan builds that machine until one hand-written program multiplies two 4×4 blocks and a second program moves tiles through the flat memory buffer. Lux stays the caller, later.

The lock is the struct in `flux9.h`:

- `T_SIZE` is 4.
- `REG_COUNT` is 16.
- Each register is an `int16_t[4][4]` stored by value (`utensor_t`).
- `memory` is a flat `uint8_t` arena.
- `pc` is the program counter.
- `stack[256]` and `sp` are a scalar address stack.

The scalar stack stays unused until a control-flow opcode exists. The sketches in `research/` stay sketches. Threads, cache pinning, packed 4-bit weights, and a lumen port wait until a 4×4 program matches a C reference.

Lux's `tensor` branch (`thecount12/lux`, `front...tensor`) is a separate POSIX `Tensor` class: variable-shape float32, one `malloc` per matrix, `matmul` as a native. flux9 does not merge into that branch. The seam is phase 5.

## Files

| File | Role |
|---|---|
| `flux9.h` | The struct above |
| `tile.c` | `matmul_4x4` and `vadd_4x4`, no bytecode |
| `vm.c` | Fetch, decode, one switch. Replaces the header copy that is there now |
| `main.c` | Load a program, run it, print `T0` |
| `prog.h` | The first program as a `uint32_t` array |

## Instruction word

One instruction is a `uint32_t`:

```
 31      24 23    20 19    16 15    12 11            0
+----------+--------+--------+--------+--------------+
| opcode   | dest   | src A  | src B  | spare / imm  |
| 8 bits   | 4 bits | 4 bits | 4 bits | 12 bits      |
+----------+--------+--------+--------+--------------+
```

Registers are `T0`–`T15`.

| Opcode | Encoding | Effect |
|---|---|---|
| `HALT` | `0x00` | Stop |
| `MOV` | `0x01` | `Td = Ta` |
| `MATMUL` | `0x02` | `Td = Ta × Tb` |
| `VADD` | `0x03` | `Td = Ta + Tb`, elementwise |
| `LD` | `0x04` | Load 32 bytes from `memory[imm]` into `Td` |
| `ST` | `0x05` | Store `Ta` to `memory[imm]` |

`LD` and `ST` arrive in phase 4. The first program uses only `MATMUL` and `HALT`.

## Phases

### 1. The tile, with no VM — done

Implement `matmul_4x4(dest, a, b)` and `vadd_4x4`. Accumulate each dot product in an `int32_t`. Elements in this phase stay in `-64..64`, so four products of that size fit in 32 bits. Store the result back as `int16_t`.

Exit test: a 4×4 multiply written as two C arrays matches the same multiply done with nested loops in a throwaway `main`. Print both matrices. Same bytes means the kernel is real.

Run: `make test` → `phase 1 passed`.

### 2. The interpreter — done

`vm.c` owns a `Flux9VM`. The loop reads `memory` as a stream of `uint32_t` at `pc` (byte offset), splits out the four fields, and switches.

- `MOV` is struct assignment.
- `MATMUL` calls the phase-1 function on `registers[d]`, `registers[a]`, and `registers[b]`.
- An unknown opcode halts.

Exit test: fill `T1` and `T2` from C, point `pc` at `{ MATMUL T0, T1, T2, HALT }`, run, and show `T0` equals the phase-1 product.

Run: `make test` → `phase 2 passed`.

### 3. A program that stands alone — done

Put the instruction words and the two input matrices in `prog.h`. On startup, copy the matrices into `T1` and `T2`, set `pc` to the first instruction, run until `HALT`, and print `T0`.

Exit test: `./flux9` prints the known product. The run itself does not call `malloc`.

Run: `make test` → prints `T0` then `phase 3 passed`.

### 4. The memory buffer — done

One `malloc` of a fixed arena (64 KiB is enough), freed at exit. `LD` and `ST` move one register (32 bytes) to a byte offset in that arena. Code and tile slots share the same arena; `imm` is a 12-bit byte offset.

Exit test: store `T1` and `T2`, load them back into `T3` and `T4`, `MATMUL` into `T0`, and match phase 1 again. `stack[]` is still untouched.

Run: `make test` → `phase 4 passed`.

### 5. Lux seam — done

Lux `tensor` merged (`front` @ `c710ebe`, PR #114). Lux keeps `Tensor(rows, cols)` and `matmul`.

Seam:
- `api.c` / `api.h` — `flux9_matmul` runs `MATMUL T0,T1,T2; HALT` for hosts.
- Lux `posix/tensor_flux9_native.c` — only file that includes both Lux `tensor.h` and `flux9.h`. Exposes `t.matmulFlux9(b)` for exact 4×4 matrices with elements in `[-64, 64]`.
- Lux Makefile: optional. Default `USE_FLUX9=0` (lux builds alone). `make USE_FLUX9=1` sets `-DLUX_FLUX9`, `FLUX9_DIR ?= ../../flux9`, and links `tile.c` / `vm.c` / `api.c`.

Exit tests:
- `cd lux/posix && make && ./lux ../examples/tensor_check.lux` → `tensor check passed`
- `make USE_FLUX9=1 && ./lux ../examples/flux9_tile.lux` → `flux9 tile check passed`

Next (not required for phase 5): tile larger matrices (multiples of 4) with int32 accumulation across K-blocks.

## Order of proof

| Phase | What it proves |
|---|---|
| 1 | The arithmetic |
| 2 | A bytecode instruction can fire it |
| 3 | The program is data |
| 4 | The register file is a window onto the arena |
| 5 | The seam with Lux |
