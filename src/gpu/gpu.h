#ifndef PUMICE_GPU_H
#define PUMICE_GPU_H

#include <stdint.h>
#include <stdbool.h>

#define GPU_SCREEN_WIDTH   1280
#define GPU_SCREEN_HEIGHT  720
#define GPU_FB_SIZE        (GPU_SCREEN_WIDTH * GPU_SCREEN_HEIGHT * 4)

#define GPU_MAX_TEXTURES   256
#define GPU_MAX_BUFFERS    256

typedef enum {
    GPU_ENGINE_2D,
    GPU_ENGINE_3D,
    GPU_ENGINE_COMPUTE,
    GPU_ENGINE_DMA,
    GPU_ENGINE_COUNT
} gpu_engine_type;

typedef enum {
    PIX_FMT_RGBA8,
    PIX_FMT_BGRA8,
    PIX_FMT_RGB565,
    PIX_FMT_RGBA16F,
    PIX_FMT_R8,
    PIX_FMT_RG8,
    PIX_FMT_BC1,
    PIX_FMT_BC3,
    PIX_FMT_BC4,
    PIX_FMT_BC5,
    PIX_FMT_ASTC_4x4,
} pixel_format;

typedef struct {
    uint64_t addr;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    pixel_format format;
    bool valid;
} gpu_texture;

typedef struct {
    uint64_t addr;
    uint32_t size;
    bool valid;
} gpu_buffer;

typedef struct {
    uint32_t regs[0x10000];
} gpu_engine;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    pixel_format format;
    uint8_t *pixels;
    bool dirty;
} gpu_framebuffer;

typedef struct {
    uint64_t put;
    uint64_t get;
    uint64_t fence_value;
    bool enabled;
} gpu_channel;

typedef struct {
    gpu_engine engines[GPU_ENGINE_COUNT];
    gpu_framebuffer fb;
    gpu_texture textures[GPU_MAX_TEXTURES];
    gpu_buffer buffers[GPU_MAX_BUFFERS];
    gpu_channel channels[8];

    uint32_t cmd_buffer[0x4000];
    uint32_t cmd_count;

    bool initialized;
} gpu_state;

void gpu_init(gpu_state *gpu);
void gpu_destroy(gpu_state *gpu);

void gpu_write_reg(gpu_state *gpu, gpu_engine_type engine, uint32_t reg, uint32_t value);
uint32_t gpu_read_reg(gpu_state *gpu, gpu_engine_type engine, uint32_t reg);

void gpu_submit_cmdlist(gpu_state *gpu, const uint32_t *cmds, uint32_t count);
void gpu_process_commands(gpu_state *gpu);

void gpu_set_framebuffer(gpu_state *gpu, uint32_t width, uint32_t height, pixel_format fmt);
const uint8_t *gpu_get_framebuffer(gpu_state *gpu);
void gpu_clear_framebuffer(gpu_state *gpu, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

int gpu_alloc_texture(gpu_state *gpu, uint64_t addr, uint32_t w, uint32_t h, pixel_format fmt);
int gpu_alloc_buffer(gpu_state *gpu, uint64_t addr, uint32_t size);

void gpu_blit_texture(gpu_state *gpu, int tex_id, int32_t dst_x, int32_t dst_y);

#endif
