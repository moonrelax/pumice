#include "memory.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

void mem_init(address_space *as) {
    memset(as, 0, sizeof(*as));
    as->heap_current = HEAP_BASE;
}

void mem_destroy(address_space *as) {
    for (int i = 0; i < as->region_count; i++) {
        if (as->regions[i].host_ptr)
            munmap(as->regions[i].host_ptr, as->regions[i].size);
    }
    memset(as, 0, sizeof(*as));
}

bool mem_map(address_space *as, uint64_t base, uint64_t size,
             uint32_t perm, const char *label) {
    if (as->region_count >= MAX_MEM_REGIONS) {
        LOG_ERR("too many memory regions");
        return false;
    }

    int prot = PROT_NONE;
    if (perm & MEM_PERM_READ)  prot |= PROT_READ;
    if (perm & MEM_PERM_WRITE) prot |= PROT_WRITE;

    uint8_t *ptr = mmap(NULL, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        LOG_ERR("mmap failed for region '%s' (0x%lx bytes)", label, size);
        return false;
    }

    mem_region *r = &as->regions[as->region_count++];
    r->base     = base;
    r->size     = size;
    r->perm     = perm;
    r->host_ptr = ptr;
    r->label    = label;

    LOG_INFO("mapped 0x%lx-0x%lx (%s)", base, base + size, label);
    return true;
}

static mem_region *mem_find(address_space *as, uint64_t addr) {
    for (int i = 0; i < as->region_count; i++) {
        mem_region *r = &as->regions[i];
        if (addr >= r->base && addr < r->base + r->size)
            return r;
    }
    return NULL;
}

uint8_t *mem_get_ptr(address_space *as, uint64_t addr) {
    mem_region *r = mem_find(as, addr);
    if (!r) return NULL;
    return r->host_ptr + (addr - r->base);
}

bool mem_read(address_space *as, uint64_t addr, void *out, size_t len) {
    uint8_t *p = mem_get_ptr(as, addr);
    if (!p) {
        LOG_ERR("read fault at 0x%lx (size %zu)", addr, len);
        return false;
    }
    memcpy(out, p, len);
    return true;
}

bool mem_write(address_space *as, uint64_t addr, const void *in, size_t len) {
    uint8_t *p = mem_get_ptr(as, addr);
    if (!p) {
        LOG_ERR("write fault at 0x%lx (size %zu)", addr, len);
        return false;
    }
    memcpy(p, in, len);
    return true;
}

uint8_t  mem_read8(address_space *as, uint64_t addr)  { uint8_t  v = 0; mem_read(as, addr, &v, 1); return v; }
uint16_t mem_read16(address_space *as, uint64_t addr) { uint16_t v = 0; mem_read(as, addr, &v, 2); return v; }
uint32_t mem_read32(address_space *as, uint64_t addr) { uint32_t v = 0; mem_read(as, addr, &v, 4); return v; }
uint64_t mem_read64(address_space *as, uint64_t addr) { uint64_t v = 0; mem_read(as, addr, &v, 8); return v; }

void mem_write8(address_space *as, uint64_t addr, uint8_t val)   { mem_write(as, addr, &val, 1); }
void mem_write16(address_space *as, uint64_t addr, uint16_t val) { mem_write(as, addr, &val, 2); }
void mem_write32(address_space *as, uint64_t addr, uint32_t val) { mem_write(as, addr, &val, 4); }
void mem_write64(address_space *as, uint64_t addr, uint64_t val) { mem_write(as, addr, &val, 8); }
