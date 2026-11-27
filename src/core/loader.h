#ifndef PUMICE_LOADER_H
#define PUMICE_LOADER_H

#include "kernel.h"
#include <stdbool.h>

typedef struct {
    uint64_t base;
    uint64_t text_start;
    uint64_t text_size;
    uint64_t ro_start;
    uint64_t ro_size;
    uint64_t data_start;
    uint64_t data_size;
    uint64_t bss_size;
    uint64_t entry;
} loaded_module;

bool loader_load_nso(kernel_state *ks, const char *path,
                     uint64_t base, loaded_module *out);
bool loader_load_nro(kernel_state *ks, const char *path,
                     uint64_t base, loaded_module *out);

#endif
