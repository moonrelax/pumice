#ifndef PUMICE_MEMORY_H
#define PUMICE_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define GUEST_PAGE_SIZE  0x1000
#define GUEST_PAGE_SHIFT 12

#define MEM_PERM_NONE  0
#define MEM_PERM_READ  (1 << 0)
#define MEM_PERM_WRITE (1 << 1)
#define MEM_PERM_EXEC  (1 << 2)
#define MEM_PERM_RW    (MEM_PERM_READ | MEM_PERM_WRITE)
#define MEM_PERM_RX    (MEM_PERM_READ | MEM_PERM_EXEC)
#define MEM_PERM_RWX   (MEM_PERM_READ | MEM_PERM_WRITE | MEM_PERM_EXEC)

#define ADDR_SPACE_BASE  0x08000000ULL
#define ADDR_SPACE_SIZE  (1ULL << 39)
#define HEAP_BASE        0x40000000ULL
#define STACK_BASE       0x70000000ULL
#define TLS_BASE         0x80000000ULL

#define MAX_MEM_REGIONS 256

typedef struct {
    uint64_t base;
    uint64_t size;
    uint32_t perm;
    uint8_t *host_ptr;
    const char *label;
} mem_region;

typedef struct {
    mem_region regions[MAX_MEM_REGIONS];
    int        region_count;
    uint64_t   heap_current;
} address_space;

void     mem_init(address_space *as);
void     mem_destroy(address_space *as);
bool     mem_map(address_space *as, uint64_t base, uint64_t size,
                 uint32_t perm, const char *label);
uint8_t *mem_get_ptr(address_space *as, uint64_t addr);
bool     mem_read(address_space *as, uint64_t addr, void *out, size_t len);
bool     mem_write(address_space *as, uint64_t addr, const void *in, size_t len);
uint8_t  mem_read8(address_space *as, uint64_t addr);
uint16_t mem_read16(address_space *as, uint64_t addr);
uint32_t mem_read32(address_space *as, uint64_t addr);
uint64_t mem_read64(address_space *as, uint64_t addr);
void     mem_write8(address_space *as, uint64_t addr, uint8_t val);
void     mem_write16(address_space *as, uint64_t addr, uint16_t val);
void     mem_write32(address_space *as, uint64_t addr, uint32_t val);
void     mem_write64(address_space *as, uint64_t addr, uint64_t val);

#endif
