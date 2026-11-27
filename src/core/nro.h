#ifndef PUMICE_NRO_H
#define PUMICE_NRO_H

#include <stdint.h>

#define NRO_MAGIC 0x304F524E /* "NRO0" */

typedef struct {
    uint32_t offset;
    uint32_t size;
} nro_segment;

typedef struct {
    uint32_t unused;
    uint32_t mod0_offset;
    uint8_t  padding[8];
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t flags;
    nro_segment text;
    nro_segment ro;
    nro_segment data;
    uint32_t bss_size;
    uint32_t reserved;
    uint8_t  build_id[0x20];
    uint8_t  reserved2[0x20];
} nro_header;

typedef struct {
    uint32_t magic;           /* "MOD0" = 0x30444F4D */
    int32_t  dynamic_offset;
    int32_t  bss_start_offset;
    int32_t  bss_end_offset;
    int32_t  eh_frame_hdr_start_offset;
    int32_t  eh_frame_hdr_end_offset;
    int32_t  module_object_offset;
} mod0_header;

#define MOD0_MAGIC 0x30444F4D

#endif
