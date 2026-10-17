# flux9

A tiny **matrix bytecode machine** in pure C. Sixteen registers, each a fixed **4×4 `int16`** tile. One instruction can multiply two tiles. Lux can optionally call into it; Lux does not require it.

Build history and phase checklist: [PLAN.md](PLAN.md).

---

## What we actually built

Not a full OS, not a BLAS, not an LLM runtime. We built three layers that stack:

| Layer | What it is |
|---|---|
| **Tile kernels** (`tile.c`) | `matmul_4x4` / `vadd_4x4` over `int16_t[4][4]` |
| **VM** (`vm.c`) | Fetch a 32-bit instruction, decode, run `HALT` / `MOV` / `MATMUL` / `VADD` / `LD` / `ST` |
| **Host API** (`api.c`) | `flux9_matmul` — load two tiles, run `MATMUL T0,T1,T2; HALT`, return `T0` |

The demo binary (`main.c` + `prog.h`) allocates a 64 KiB arena, stores `T1`/`T2` into memory, loads them into `T3`/`T4`, multiplies into `T0`, and checks the known product.

```
  T0 … T15     each = 4×4 int16  (32 bytes)
  memory[]     flat arena (code + tile slots)
  pc           byte offset into memory
  stack[256]   reserved for later control flow (unused today)
```

**Opcodes**

| Op | Code | Meaning |
|---|---|---|
| `HALT` | `0x00` | stop |
| `MOV` | `0x01` | `Td = Ta` |
| `MATMUL` | `0x02` | `Td = Ta × Tb` |
| `VADD` | `0x03` | `Td = Ta + Tb` elementwise |
| `LD` | `0x04` | `Td ← memory[imm]` (32 bytes) |
| `ST` | `0x05` | `memory[imm] ← Ta` |

Instruction word: `[opcode:8][dest:4][srcA:4][srcB:4][imm:12]`. Helpers: `ENC(...)`, `ENC_IMM(...)`.

**Hard limits (by design today)**

- Tile shape is always 4×4 (`T_SIZE`).
- Elements should stay in **[-64, 64]** so a 4-wide int16×int16 sum fits in int32.
- `LD`/`ST` `imm` is 12-bit (first 4 KiB of the arena).
- No floats inside the VM. Floats only appear at the Lux boundary (convert in, convert out).

---

## How to use it: standalone

```bash
cd flux9
make          # builds ./flux9
make test     # runs the arena program; expects "phase 4 passed"
./flux9       # prints T0, then the pass/fail line
```

You should see the product matrix:

```text
     1    13     2    12
     9    29     2    28
    17    45     2    44
    25    61     2    60
phase 4 passed
```

To change the demo: edit `prog.h` (matrices + bytecode), rebuild.

---

## How to use it: from C

```c
#include "api.h"

utensor_t A, B, C;
/* fill A.data[][] and B.data[][] with int16 values in [-64, 64] */
flux9_matmul(&C, &A, &B);
/* C is T0 after MATMUL T0,T1,T2 ; HALT */
```

Link `tile.c`, `vm.c`, and `api.c`. That path does **not** need the 64 KiB arena; `flux9_matmul` keeps a tiny code array on the stack and uses registers only.

For a full program (code + `LD`/`ST` in one arena), copy the pattern in `main.c`: `malloc(ARENA_SIZE)`, copy bytecode to offset 0, set `vm.memory` / `vm.pc`, fill registers, `flux9_run`.

---

## How to use it: from Lux (optional)

Lux’s normal `Tensor` path is float32 and does **not** need flux9.

```bash
# Lux alone (no flux9)
cd ../lux/posix
make
./lux ../examples/tensor_check.lux
```

```bash
# Lux + flux9 seam (sibling checkout at ../../flux9)
make USE_FLUX9=1
./lux ../examples/flux9_tile.lux
```

Override the path if needed: `make USE_FLUX9=1 FLUX9_DIR=/path/to/flux9`.

In Lux scripts, only when built with `USE_FLUX9=1`:

```lux
var A = Tensor(4, 4);
var B = Tensor(4, 4);
/* fill with numbers in [-64, 64] */

var C = A.matmulFlux9(B);   /* runs flux9 bytecode */
var D = A.matmul(B);        /* float32 C path — always available */

C.show("via flux9");
```

Rules for `matmulFlux9`:

- Both sides must be exactly **4×4**.
- Values must round into **[-64, 64]**; otherwise the call returns `nil`.
- Result is a new float `Tensor` (int16 products written back as floats).

Without `USE_FLUX9=1`, `matmulFlux9` is not registered (`Undefined property 'matmulFlux9'`).

---

## How the pieces relate

```text
  .lux script
       │
       │  Tensor.matmul        Tensor.matmulFlux9   (USE_FLUX9=1)
       ▼                            ▼
  lux tensor.c (float32)     tensor_flux9_native.c
                                    │
                                    ▼
                              flux9_matmul (api.c)
                                    │
                              vm.c  →  tile.c
```

flux9 stays its own repo. Lux optionally **compiles** three flux9 `.c` files into the `lux` binary. There is no runtime shared library and no requirement to install flux9 for a normal Lux build.

---

## Practical “what do I do next?”

| Goal | Do this |
|---|---|
| Confirm the machine works | `make test` in this repo |
| Tinker with bytecode | Edit `prog.h`, `make test` |
| Call from your own C | `#include "api.h"`, link `tile.o vm.o api.o` |
| Call from Lux | Sibling dirs, `make USE_FLUX9=1`, use `matmulFlux9` |
| Bigger matrices | Not done yet — need tiling (multiples of 4) + accumulation; see PLAN.md “Next” |
| LLM / lumen weights | Still in `llmresearch`; flux9 is only the 4×4 tile engine so far |

---

## Repo map

| File | Role |
|---|---|
| `flux9.h` | Registers, arena, opcodes, `ENC` |
| `tile.c` / `tile.h` | Fixed 4×4 math |
| `vm.c` / `vm.h` | Interpreter |
| `api.c` / `api.h` | Host entry (`flux9_matmul`) |
| `prog.h` | Demo matrices + bytecode |
| `main.c` | Arena demo / exit test |
| `research/` | Earlier sketches — not the machine |
| `PLAN.md` | Phase-by-phase build record |
