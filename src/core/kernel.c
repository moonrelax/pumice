#include "kernel.h"
#include "svc.h"
#include "log.h"
#include <string.h>

void kernel_init(kernel_state *ks, address_space *as) {
    memset(ks, 0, sizeof(*ks));
    ks->as = as;
    ks->process_id = 1;
    ks->heap_base = HEAP_BASE;
    ks->next_handle = 1;
}

uint32_t kernel_alloc_handle(kernel_state *ks, handle_type type, void *data) {
    for (int i = ks->next_handle; i < MAX_HANDLES; i++) {
        if (ks->handles[i].type == HANDLE_NONE) {
            ks->handles[i].type = type;
            ks->handles[i].data = data;
            ks->next_handle = i + 1;
            return (uint32_t)i;
        }
    }
    return 0;
}

int kernel_create_thread(kernel_state *ks, uint64_t entry,
                         uint64_t arg, uint64_t stack_top, int priority) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (ks->threads[i].state == THREAD_FREE) {
            thread *t = &ks->threads[i];
            t->id = i;
            t->priority = priority;
            t->state = THREAD_READY;

            arm64_init(&t->cpu, ks->as);
            arm64_set_entry(&t->cpu, entry, stack_top);
            t->cpu.x[0] = arg;

            uint64_t tls = TLS_BASE + (uint64_t)i * 0x1000;
            mem_map(ks->as, tls, 0x1000, MEM_PERM_RW, "tls");
            t->tls_addr = tls;
            t->cpu.tpidr_el0 = tls;

            t->handle = kernel_alloc_handle(ks, HANDLE_THREAD, t);
            ks->thread_count++;

            LOG_INFO("created thread %d, entry=0x%lx sp=0x%lx", i, entry, stack_top);
            return i;
        }
    }
    return -1;
}

void kernel_run(kernel_state *ks) {
    while (true) {
        int found = -1;
        for (int i = 0; i < MAX_THREADS; i++) {
            if (ks->threads[i].state == THREAD_READY ||
                ks->threads[i].state == THREAD_RUNNING) {
                found = i;
                break;
            }
        }
        if (found < 0) {
            LOG_INFO("no runnable threads, stopping");
            break;
        }

        ks->current_thread = found;
        thread *t = &ks->threads[found];
        t->state = THREAD_RUNNING;

        int result = arm64_run(&t->cpu, 100000);

        switch (result) {
        case CPU_SVC:
            svc_handle(ks, &t->cpu);
            break;
        case CPU_BREAK:
            LOG_INFO("thread %d hit breakpoint at 0x%lx", found, t->cpu.pc);
            t->state = THREAD_DEAD;
            break;
        case CPU_FAULT:
            LOG_ERR("thread %d faulted at 0x%lx", found, t->cpu.pc);
            t->state = THREAD_DEAD;
            break;
        case CPU_HALTED:
            t->state = THREAD_DEAD;
            break;
        case CPU_OK:
            break;
        }

        if (t->state == THREAD_DEAD) {
            bool any_alive = false;
            for (int i = 0; i < MAX_THREADS; i++) {
                if (ks->threads[i].state == THREAD_READY ||
                    ks->threads[i].state == THREAD_RUNNING) {
                    any_alive = true;
                    break;
                }
            }
            if (!any_alive) break;
        }
    }
}
