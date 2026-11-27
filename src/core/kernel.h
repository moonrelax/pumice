#ifndef PUMICE_KERNEL_H
#define PUMICE_KERNEL_H

#include "arm64.h"
#include "memory.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_THREADS 64
#define MAX_HANDLES 1024
#define THREAD_STACK_SIZE (1 << 20)

typedef enum {
    HANDLE_NONE = 0,
    HANDLE_THREAD,
    HANDLE_SESSION,
    HANDLE_PORT,
    HANDLE_TRANSFER_MEMORY,
    HANDLE_EVENT,
} handle_type;

typedef struct {
    handle_type type;
    int         ref;
    void       *data;
} kernel_handle;

typedef enum {
    THREAD_FREE = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_WAITING,
    THREAD_DEAD,
} thread_state;

typedef struct {
    arm64_cpu    cpu;
    thread_state state;
    int          id;
    int          priority;
    uint32_t     handle;
    uint64_t     tls_addr;
} thread;

typedef struct kernel_state {
    address_space *as;
    thread         threads[MAX_THREADS];
    int            current_thread;
    int            thread_count;
    kernel_handle  handles[MAX_HANDLES];
    int            next_handle;
    uint64_t       process_id;
    uint64_t       heap_size;
    uint64_t       heap_base;
} kernel_state;

void     kernel_init(kernel_state *ks, address_space *as);
int      kernel_create_thread(kernel_state *ks, uint64_t entry,
                              uint64_t arg, uint64_t stack_top, int priority);
void     kernel_run(kernel_state *ks);
uint32_t kernel_alloc_handle(kernel_state *ks, handle_type type, void *data);

#endif
