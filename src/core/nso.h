#ifndef PUMICE_NSO_H
#define PUMICE_NSO_H

#include <stdint.h>

#define NSO_MAGIC 0x304F534E /* "NSO0" */

typedef struct {
    uint32_t file_offset;
    uint32_t memory_offset;
    uint32_t decompressed_size;
} nso_segment_header;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t reserved0;
    uint32_t flags;
    nso_segment_header text;
    uint32_t module_name_offset;
    nso_segment_header ro;
    uint32_t module_name_size;
    nso_segment_header data;
    uint32_t bss_size;
    uint8_t  build_id[0x20];
    uint32_t text_compressed_size;
    uint32_t ro_compressed_size;
    uint32_t data_compressed_size;
    uint8_t  reserved1[0x1C];
    uint64_t api_info_offset;
    uint64_t api_info_size;
    uint64_t dynstr_offset;
    uint64_t dynstr_size;
    uint64_t dynsym_offset;
    uint64_t dynsym_size;
    uint8_t  text_hash[0x20];
    uint8_t  ro_hash[0x20];
    uint8_t  data_hash[0x20];
} nso_header;

#define NSO_FLAG_TEXT_COMPRESSED (1 << 0)
#define NSO_FLAG_RO_COMPRESSED   (1 << 1)
#define NSO_FLAG_DATA_COMPRESSED (1 << 2)
#define NSO_FLAG_TEXT_HASH_CHECK (1 << 3)
#define NSO_FLAG_RO_HASH_CHECK   (1 << 4)
#define NSO_FLAG_DATA_HASH_CHECK (1 << 5)

#endif
