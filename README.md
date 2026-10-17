# flux9

A tiny **matrix bytecode machine** in pure C. Sixteen registers, each a fixed **4×4 `int16`** tile. One instruction can multiply two tiles. A host-side **tiled matmul** covers larger matrices (dims multiple of 4). Lux can optionally call into it; Lux does not require it.

Build history and phase checklist: [PLAN.md](PLAN.md).

---

## What we actually built

Not a full OS, not a BLAS, not an LLM runtime. Layers that stack:

| Layer | What it is |
|---|---|
| **Tile kernels** (`tile.c`) | `matmul_4x4` / `vadd_4x4` over `int16_t[4][4]` |
| **VM** (`vm.c`) | Fetch a 32-bit instruction, decode, run `HALT` / `MOV` / `MATMUL` / `VADD` / `LD` / `ST` |
| **Pack** (`pack.c`) | `fill_tile` / `write_tile` for row-major int16 buffers |
| **Host API** (`api.c`) | `flux9_matmul` (one tile via bytecode); `flux9_matmul_tiled` (large GEMM via tiles) |

The demo binary runs the arena program (phase 4) and an 8×8 tiled check (phase 6).

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

- Native register tile is always 4×4 (`T_SIZE`).
- `flux9_matmul_tiled` requires **M, N, K multiples of 4**; rejects others with `-1`.
- Per-tile products use int32 inside `matmul_4x4`; tiled path accumulates those in **int32**, then saturates to int16.
- Element values for Lux conversion stay in **[-64, 64]** by convention.
- `LD`/`ST` `imm` is 12-bit (first 4 KiB of the arena).
- No floats inside the VM. Floats only appear at the Lux boundary.

---

## How to use it: standalone

```bash
cd flux9
make          # builds ./flux9
make test     # phase 4 + phase 6
./flux9
```

Expect both:

```text
phase 4 passed
phase 6 passed
```

---

## How to use it: from C

**One 4×4 tile (bytecode):**

```c
#include "api.h"

utensor_t A, B, C;
flux9_matmul(&C, &A, &B);
```

**Larger matrices (tiled; dims multiple of 4):**

```c
#include "api.h"

int16_t A[M*K], B[K*N], C[M*N];
/* fill A, B row-major */
if (flux9_matmul_tiled(C, A, B, M, N, K) != 0) {
    /* bad dims — not multiples of 4 */
}
```

Link `tile.c`, `pack.c`, `vm.c`, and `api.c`.

The tiled path calls `matmul_4x4` per block (same kernel as `OP_MATMUL`). It does **not** re-enter the bytecode VM on every tile.

---

## How to use it: from Lux (optional)

Lux’s normal `Tensor` path is float32 and does **not** need flux9.

```bash
cd ../lux/posix
make                          # Lux alone
make USE_FLUX9=1              # + flux9 (FLUX9_DIR ?= ../../flux9)
./lux ../examples/flux9_tile.lux
./lux ../examples/flux9_tiled.lux   # 8×8 via tiled API
```

```lux
var A = Tensor(8, 8);
var B = Tensor(8, 8);
/* fill with numbers in [-64, 64]; dims multiple of 4 */

var C = A.matmulFlux9(B);   /* tiled flux9 path when dims > 4×4 */
var D = A.matmul(B);        /* float32 — always available */
```

Rules for `matmulFlux9`:

- Rows/cols of both sides must be **multiples of 4**, and `A.cols == B.rows`.
- Values must round into **[-64, 64]**; otherwise `nil`.
- Exact 4×4 still uses the single-tile bytecode path; larger uses `flux9_matmul_tiled`.

Without `USE_FLUX9=1`, `matmulFlux9` is not registered.

---

## How the pieces relate

```text
  .lux script
       │
       │  Tensor.matmul        Tensor.matmulFlux9   (USE_FLUX9=1)
       ▼                            ▼
  lux tensor.c (float32)     tensor_flux9_native.c
                         ┌──────────┴──────────┐
                         ▼                     ▼
                  flux9_matmul          flux9_matmul_tiled
                  (4×4 bytecode)        (pack + matmul_4x4 loop)
                         │                     │
                         └─────────► tile.c ◄──┘
```

---

## Further consideration

Pinned for later — see also [PLAN.md](PLAN.md). Classical tiled GEMM is the shipping path.

| Idea | Role vs flux9 |
|---|---|
| **Strassen** (2×2 in 7 mults) | Host recursion above flux9, or leaf rewrite of small blocks |
| **Flip-graph / Moosbauer–Poole schemes** ([arXiv:2502.04514](https://arxiv.org/abs/2502.04514)) | Better bilinear schemes for 5×5 / 6×6; possible future leaf kernels or alternate `T_SIZE` |
| **Threads / Plan 9 nodes** | Partition `(i,j)` output tiles across workers |
| **int8 / 4-bit packs** | Density pass after tiled int16 is trusted |

Tiling answers coverage of large matrices. Algebraic schemes answer fewer multiplies **per block**. They compose: scheduler stays; leaf kernel can change.

---

## Practical “what do I do next?”

| Goal | Do this |
|---|---|
| Confirm the machine works | `make test` |
| Tinker with bytecode | Edit `prog.h`, `make test` |
| Large int16 GEMM from C | `flux9_matmul_tiled` |
| Call from Lux | `make USE_FLUX9=1`, `matmulFlux9` |
| LLM / lumen weights | Still `llmresearch`; feed panels through tiled API when ready |

---

## Repo map

| File | Role |
|---|---|
| `flux9.h` | Registers, arena, opcodes, `ENC` |
| `tile.c` / `tile.h` | Fixed 4×4 math |
| `pack.c` / `pack.h` | Matrix ↔ tile slicing |
| `vm.c` / `vm.h` | Interpreter |
| `api.c` / `api.h` | `flux9_matmul`, `flux9_matmul_tiled` |
| `prog.h` | Demo matrices + bytecode |
| `main.c` | Phase 4 + phase 6 exit tests |
| `research/` | Earlier sketches — not the machine |
| `PLAN.md` | Phase-by-phase build record |
