#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>

void* worker_thread_entry(void* arg) {
    VMWorkerContext* ctx = (VMWorkerContext*)arg;

    // Pin thread to physical core ID
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(ctx->core_id, &cpuset);
    
    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);

    // Enter the zero-overhead execution loop
    vm_worker_loop(ctx);

    return NULL;
}