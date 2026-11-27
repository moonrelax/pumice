#include "svc.h"
#include "kernel.h"
#include "log.h"
#include <string.h>

static void svc_set_heap_size(kernel_state *ks, arm64_cpu *cpu) {
    uint64_t size = cpu->x[1];

    if (ks->heap_size == 0 && size > 0) {
        mem_map(ks->as, ks->heap_base, size, MEM_PERM_RW, "heap");
    }
    ks->heap_size = size;

    cpu->x[0] = RESULT_SUCCESS;
    cpu->x[1] = ks->heap_base;
}

static void svc_exit_process(kernel_state *ks, arm64_cpu *cpu) {
    LOG_INFO("process exit requested");
    for (int i = 0; i < MAX_THREADS; i++)
        ks->threads[i].state = THREAD_DEAD;
    cpu->running = false;
}

static void svc_exit_thread(kernel_state *ks, arm64_cpu *cpu) {
    int tid = ks->current_thread;
    LOG_INFO("thread %d exit", tid);
    ks->threads[tid].state = THREAD_DEAD;
    cpu->running = false;
}

static void svc_sleep_thread(kernel_state *ks, arm64_cpu *cpu) {
    (void)ks;
    int64_t ns = (int64_t)cpu->x[0];
    if (ns == 0) {
        /* yield */
    }
    cpu->x[0] = RESULT_SUCCESS;
}

static void svc_close_handle(kernel_state *ks, arm64_cpu *cpu) {
    uint32_t h = (uint32_t)cpu->x[0];
    if (h < MAX_HANDLES) {
        ks->handles[h].type = HANDLE_NONE;
        ks->handles[h].data = NULL;
        cpu->x[0] = RESULT_SUCCESS;
    } else {
        cpu->x[0] = RESULT_INVALID_HANDLE;
    }
}

static void svc_get_info(kernel_state *ks, arm64_cpu *cpu) {
    uint32_t id1 = (uint32_t)cpu->x[1];
    uint64_t handle = cpu->x[2];
    uint64_t id2 = cpu->x[3];
    (void)handle;

    uint64_t out = 0;
    switch (id1) {
    case 0:  out = 0; break;                              /* AllowedCpuIdBitmask */
    case 2:  out = ADDR_SPACE_BASE; break;                /* MapRegionBaseAddr */
    case 3:  out = ADDR_SPACE_SIZE; break;                /* MapRegionSize */
    case 4:  out = HEAP_BASE; break;                      /* HeapRegionBaseAddr */
    case 5:  out = ks->heap_size; break;                  /* HeapRegionSize */
    case 6:  out = 0x200000; break;                       /* TotalMemorySize */
    case 7:  out = 0x100000; break;                       /* UsedMemorySize */
    case 11: out = 0; break;                              /* RandomEntropy */
    case 12: out = ADDR_SPACE_BASE; break;                /* AslrRegionBaseAddr */
    case 13: out = ADDR_SPACE_SIZE; break;                /* AslrRegionSize */
    case 14: out = STACK_BASE; break;                     /* StackRegionBaseAddr */
    case 15: out = THREAD_STACK_SIZE * MAX_THREADS; break;/* StackRegionSize */
    default:
        LOG_INFO("GetInfo: unhandled id1=%u id2=%lu", id1, id2);
        break;
    }
    cpu->x[0] = RESULT_SUCCESS;
    cpu->x[1] = out;
}

static void svc_connect_to_named_port(kernel_state *ks, arm64_cpu *cpu) {
    uint64_t name_addr = cpu->x[1];
    char name[12] = {0};
    for (int i = 0; i < 11; i++) {
        name[i] = (char)mem_read8(ks->as, name_addr + i);
        if (!name[i]) break;
    }

    uint32_t h = kernel_alloc_handle(ks, HANDLE_SESSION, NULL);
    LOG_INFO("ConnectToNamedPort('%s') -> handle %u", name, h);
    cpu->x[0] = RESULT_SUCCESS;
    cpu->x[1] = h;
}

static void svc_send_sync_request(kernel_state *ks, arm64_cpu *cpu) {
    (void)ks;
    LOG_TRACE("SendSyncRequest handle=%u", (uint32_t)cpu->x[0]);
    cpu->x[0] = RESULT_SUCCESS;
}

static void svc_output_debug_string(kernel_state *ks, arm64_cpu *cpu) {
    uint64_t addr = cpu->x[0];
    uint64_t len = cpu->x[1];
    if (len > 256) len = 256;

    char buf[257] = {0};
    for (uint64_t i = 0; i < len; i++)
        buf[i] = (char)mem_read8(ks->as, addr + i);

    LOG_INFO("debug: %s", buf);
    cpu->x[0] = RESULT_SUCCESS;
}

static void svc_break(kernel_state *ks, arm64_cpu *cpu) {
    (void)ks;
    uint64_t reason = cpu->x[0];
    LOG_INFO("svc break: reason=0x%lx", reason);
    cpu->running = false;
}

static void svc_get_process_id(kernel_state *ks, arm64_cpu *cpu) {
    cpu->x[0] = RESULT_SUCCESS;
    cpu->x[1] = ks->process_id;
}

static void svc_get_thread_id(kernel_state *ks, arm64_cpu *cpu) {
    cpu->x[0] = RESULT_SUCCESS;
    cpu->x[1] = (uint64_t)ks->current_thread;
}

void svc_handle(kernel_state *ks, arm64_cpu *cpu) {
    uint32_t id = (uint32_t)cpu->svc_id;

    switch (id) {
    case SVC_SET_HEAP_SIZE:          svc_set_heap_size(ks, cpu); break;
    case SVC_EXIT_PROCESS:           svc_exit_process(ks, cpu); break;
    case SVC_EXIT_THREAD:            svc_exit_thread(ks, cpu); break;
    case SVC_SLEEP_THREAD:           svc_sleep_thread(ks, cpu); break;
    case SVC_CLOSE_HANDLE:           svc_close_handle(ks, cpu); break;
    case SVC_CONNECT_TO_NAMED_PORT:  svc_connect_to_named_port(ks, cpu); break;
    case SVC_SEND_SYNC_REQUEST:      svc_send_sync_request(ks, cpu); break;
    case SVC_GET_PROCESS_ID:         svc_get_process_id(ks, cpu); break;
    case SVC_GET_THREAD_ID:          svc_get_thread_id(ks, cpu); break;
    case SVC_BREAK:                  svc_break(ks, cpu); break;
    case SVC_OUTPUT_DEBUG_STRING:    svc_output_debug_string(ks, cpu); break;
    case SVC_GET_INFO:               svc_get_info(ks, cpu); break;
    default:
        LOG_ERR("unhandled SVC 0x%02X", id);
        cpu->x[0] = RESULT_SUCCESS;
        break;
    }
}
