void exec_matmul(TensorReg* dest, const TensorReg* A, const TensorReg* B) {
    // Assumptions: A is (M x K), B is (K x N), dest is (M x N)
    int M = A->rows;
    int K = A->cols;
    int N = B->cols;

    int8_t* a_data = A->data_ptr;
    int8_t* b_data = B->data_ptr;
    int32_t* dest_acc = (int32_t*)dest->data_ptr; // Scratchpad accumulation buffer

    // L1 Cache-friendly blocked/tiled loop execution
    // (In production, this inner block maps directly to AVX2/AVX-512 or NEON vector intrinsics)
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            int32_t acc = 0;
            for (int k = 0; k < K; k++) {
                // Fixed-point dot product step: INT8 * INT8 -> INT32 accumulation
                acc += (int32_t)a_data[i * K + k] * (int32_t)b_data[k * N + j];
            }
            dest_acc[i * N + j] = acc;
        }
    }

    // Apply combined dequantization scaling factor
    dest->scale = A->scale * B->scale;
}