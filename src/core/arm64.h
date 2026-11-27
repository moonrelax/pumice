#ifndef PUMICE_ARM64_H
#define PUMICE_ARM64_H

#include <stdint.h>
#include <stdbool.h>
#include "memory.h"

#define NUM_GREGS 31
#define NUM_VREGS 32

typedef struct {
    uint64_t x[NUM_GREGS];
    uint64_t sp;
    uint64_t pc;
    uint32_t nzcv;
    uint64_t tpidr_el0;

    __uint128_t v[NUM_VREGS];
    uint32_t    fpcr;
    uint32_t    fpsr;

    address_space *as;
    bool          running;
    uint64_t      svc_id;
} arm64_cpu;

#define NZCV_N (1u << 31)
#define NZCV_Z (1u << 30)
#define NZCV_C (1u << 29)
#define NZCV_V (1u << 28)

void arm64_init(arm64_cpu *cpu, address_space *as);
void arm64_set_entry(arm64_cpu *cpu, uint64_t pc, uint64_t sp);
int  arm64_step(arm64_cpu *cpu);
int  arm64_run(arm64_cpu *cpu, uint64_t max_steps);

#define CPU_OK       0
#define CPU_SVC      1
#define CPU_BREAK    2
#define CPU_FAULT    3
#define CPU_HALTED   4

#endif
