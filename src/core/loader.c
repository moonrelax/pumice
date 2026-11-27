#include "loader.h"
#include "nso.h"
#include "nro.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *read_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(sz);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    fclose(f);
    *out_size = (size_t)sz;
    return buf;
}

bool loader_load_nso(kernel_state *ks, const char *path,
                     uint64_t base, loaded_module *out) {
    size_t file_size;
    uint8_t *data = read_file(path, &file_size);
    if (!data) {
        LOG_ERR("failed to open '%s'", path);
        return false;
    }

    if (file_size < sizeof(nso_header)) {
        LOG_ERR("file too small for NSO header");
        free(data);
        return false;
    }

    nso_header *hdr = (nso_header *)data;
    if (hdr->magic != NSO_MAGIC) {
        LOG_ERR("bad NSO magic: 0x%08X", hdr->magic);
        free(data);
        return false;
    }

    uint64_t text_off = hdr->text.memory_offset;
    uint64_t ro_off   = hdr->ro.memory_offset;
    uint64_t data_off = hdr->data.memory_offset;
    uint64_t text_sz  = hdr->text.decompressed_size;
    uint64_t ro_sz    = hdr->ro.decompressed_size;
    uint64_t data_sz  = hdr->data.decompressed_size;
    uint64_t bss_sz   = hdr->bss_size;

    uint64_t total = data_off + data_sz + bss_sz;
    total = (total + 0xFFF) & ~0xFFFULL;

    if (!mem_map(ks->as, base, total, MEM_PERM_RWX, "nso")) {
        free(data);
        return false;
    }

    /* For now, only uncompressed segments */
    if (!(hdr->flags & NSO_FLAG_TEXT_COMPRESSED)) {
        mem_write(ks->as, base + text_off,
                  data + hdr->text.file_offset, text_sz);
    } else {
        LOG_ERR("compressed NSO segments not yet supported");
        free(data);
        return false;
    }

    if (!(hdr->flags & NSO_FLAG_RO_COMPRESSED)) {
        mem_write(ks->as, base + ro_off,
                  data + hdr->ro.file_offset, ro_sz);
    }

    if (!(hdr->flags & NSO_FLAG_DATA_COMPRESSED)) {
        mem_write(ks->as, base + data_off,
                  data + hdr->data.file_offset, data_sz);
    }

    out->base       = base;
    out->text_start = base + text_off;
    out->text_size  = text_sz;
    out->ro_start   = base + ro_off;
    out->ro_size    = ro_sz;
    out->data_start = base + data_off;
    out->data_size  = data_sz;
    out->bss_size   = bss_sz;
    out->entry      = base + text_off;

    LOG_INFO("loaded NSO '%s' at 0x%lx (text=%lu ro=%lu data=%lu bss=%lu)",
             path, base, text_sz, ro_sz, data_sz, bss_sz);

    free(data);
    return true;
}

bool loader_load_nro(kernel_state *ks, const char *path,
                     uint64_t base, loaded_module *out) {
    size_t file_size;
    uint8_t *data = read_file(path, &file_size);
    if (!data) {
        LOG_ERR("failed to open '%s'", path);
        return false;
    }

    if (file_size < sizeof(nro_header)) {
        LOG_ERR("file too small for NRO header");
        free(data);
        return false;
    }

    nro_header *hdr = (nro_header *)data;
    if (hdr->magic != NRO_MAGIC) {
        LOG_ERR("bad NRO magic: 0x%08X", hdr->magic);
        free(data);
        return false;
    }

    uint64_t text_off = hdr->text.offset;
    uint64_t text_sz  = hdr->text.size;
    uint64_t ro_off   = hdr->ro.offset;
    uint64_t ro_sz    = hdr->ro.size;
    uint64_t data_off = hdr->data.offset;
    uint64_t data_sz  = hdr->data.size;
    uint64_t bss_sz   = hdr->bss_size;

    uint64_t total = hdr->size + bss_sz;
    total = (total + 0xFFF) & ~0xFFFULL;

    if (!mem_map(ks->as, base, total, MEM_PERM_RWX, "nro")) {
        free(data);
        return false;
    }

    mem_write(ks->as, base + text_off, data + text_off, text_sz);
    mem_write(ks->as, base + ro_off, data + ro_off, ro_sz);
    mem_write(ks->as, base + data_off, data + data_off, data_sz);

    out->base       = base;
    out->text_start = base + text_off;
    out->text_size  = text_sz;
    out->ro_start   = base + ro_off;
    out->ro_size    = ro_sz;
    out->data_start = base + data_off;
    out->data_size  = data_sz;
    out->bss_size   = bss_sz;
    out->entry      = base + text_off;

    LOG_INFO("loaded NRO '%s' at 0x%lx (text=%lu ro=%lu data=%lu bss=%lu)",
             path, base, text_sz, ro_sz, data_sz, bss_sz);

    free(data);
    return true;
}
