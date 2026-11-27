#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/memory.h"
#include "core/kernel.h"
#include "core/loader.h"
#include "core/log.h"
#include "hle/service.h"
#include "gpu/gpu.h"
#include "gpu/display.h"
#include "audio/audio.h"
#include "input/hid.h"
#include "fs/fs.h"

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s <file.nso|file.nro>\n", prog);
    exit(1);
}

int main(int argc, char **argv) {
    if (argc < 2) usage(argv[0]);

    const char *rom_path = argv[1];
    const char *ext = strrchr(rom_path, '.');
    if (!ext) {
        LOG_ERR("cannot determine file type (no extension)");
        return 1;
    }

    LOG_INFO("pumice - experimental nintendo switch emulator");
    LOG_INFO("loading '%s'", rom_path);

    address_space as;
    mem_init(&as);

    kernel_state ks;
    kernel_init(&ks, &as);

    service_manager sm;
    sm_init(&sm);
    hle_register_all(&sm);

    gpu_state gpu;
    gpu_init(&gpu);

    display_state display;
    display_init(&display, &gpu);

    audio_state audio;
    audio_init(&audio);

    hid_state hid;
    hid_init(&hid);

    fs_state fs;
    fs_init(&fs, ".");

    loaded_module mod = {0};
    bool ok;

    if (strcmp(ext, ".nso") == 0) {
        ok = loader_load_nso(&ks, rom_path, ADDR_SPACE_BASE, &mod);
    } else if (strcmp(ext, ".nro") == 0) {
        ok = loader_load_nro(&ks, rom_path, ADDR_SPACE_BASE, &mod);
    } else {
        LOG_ERR("unsupported file type '%s'", ext);
        return 1;
    }

    if (!ok) {
        LOG_ERR("failed to load '%s'", rom_path);
        mem_destroy(&as);
        return 1;
    }

    uint64_t stack_base = STACK_BASE;
    uint64_t stack_size = 1 << 20;
    mem_map(&as, stack_base, stack_size, MEM_PERM_RW, "main-stack");

    int tid = kernel_create_thread(&ks, mod.entry, 0,
                                   stack_base + stack_size, 0);
    if (tid < 0) {
        LOG_ERR("failed to create main thread");
        mem_destroy(&as);
        return 1;
    }

    LOG_INFO("starting execution at 0x%lx", mod.entry);
    kernel_run(&ks);
    LOG_INFO("execution finished");

    gpu_destroy(&gpu);
    audio_destroy(&audio);
    fs_destroy(&fs);
    mem_destroy(&as);
    return 0;
}
