#include "gpu.h"
#include "../core/log.h"
#include <stdlib.h>
#include <string.h>

void gpu_init(gpu_state *gpu) {
    memset(gpu, 0, sizeof(*gpu));
    gpu->fb.width = GPU_SCREEN_WIDTH;
    gpu->fb.height = GPU_SCREEN_HEIGHT;
    gpu->fb.stride = GPU_SCREEN_WIDTH * 4;
    gpu->fb.format = PIX_FMT_RGBA8;
    gpu->fb.pixels = calloc(1, GPU_FB_SIZE);
    gpu->initialized = true;
    LOG_INFO("gpu: initialized %ux%u framebuffer", gpu->fb.width, gpu->fb.height);
}

void gpu_destroy(gpu_state *gpu) {
    free(gpu->fb.pixels);
    gpu->fb.pixels = NULL;
    gpu->initialized = false;
}

void gpu_write_reg(gpu_state *gpu, gpu_engine_type engine, uint32_t reg, uint32_t value) {
    if (engine >= GPU_ENGINE_COUNT) return;
    if (reg >= 0x10000) return;
    gpu->engines[engine].regs[reg] = value;

    if (engine == GPU_ENGINE_2D) {
        switch (reg) {
        case 0x0200: /* 2D blit trigger */
            LOG_TRACE("gpu: 2D blit triggered");
            break;
        case 0x0220: /* clear color */
            {
                uint8_t r = value & 0xFF;
                uint8_t g = (value >> 8) & 0xFF;
                uint8_t b = (value >> 16) & 0xFF;
                uint8_t a = (value >> 24) & 0xFF;
                gpu_clear_framebuffer(gpu, r, g, b, a);
            }
            break;
        }
    } else if (engine == GPU_ENGINE_3D) {
        switch (reg) {
        case 0x0001: /* 3D draw trigger */
            LOG_TRACE("gpu: 3D draw call");
            break;
        case 0x0100: /* viewport width */
            LOG_TRACE("gpu: viewport width = %u", value);
            break;
        case 0x0101: /* viewport height */
            LOG_TRACE("gpu: viewport height = %u", value);
            break;
        }
    } else if (engine == GPU_ENGINE_DMA) {
        switch (reg) {
        case 0x0300: /* DMA launch */
            LOG_TRACE("gpu: DMA transfer triggered");
            break;
        }
    }
}

uint32_t gpu_read_reg(gpu_state *gpu, gpu_engine_type engine, uint32_t reg) {
    if (engine >= GPU_ENGINE_COUNT || reg >= 0x10000) return 0;
    return gpu->engines[engine].regs[reg];
}

void gpu_submit_cmdlist(gpu_state *gpu, const uint32_t *cmds, uint32_t count) {
    if (count > 0x4000) count = 0x4000;
    memcpy(gpu->cmd_buffer, cmds, count * sizeof(uint32_t));
    gpu->cmd_count = count;
    gpu_process_commands(gpu);
}

void gpu_process_commands(gpu_state *gpu) {
    uint32_t i = 0;
    while (i < gpu->cmd_count) {
        uint32_t header = gpu->cmd_buffer[i++];
        uint32_t method = header & 0xFFF;
        uint32_t subchannel = (header >> 13) & 0x7;
        uint32_t count = (header >> 16) & 0x1FFF;
        uint32_t mode = (header >> 29) & 0x7;
        (void)mode;

        gpu_engine_type engine;
        switch (subchannel) {
        case 0: engine = GPU_ENGINE_3D; break;
        case 1: engine = GPU_ENGINE_2D; break;
        case 2: engine = GPU_ENGINE_COMPUTE; break;
        case 3: engine = GPU_ENGINE_DMA; break;
        default: engine = GPU_ENGINE_3D; break;
        }

        for (uint32_t j = 0; j < count && i < gpu->cmd_count; j++, i++) {
            gpu_write_reg(gpu, engine, method + j, gpu->cmd_buffer[i]);
        }
    }
    gpu->cmd_count = 0;
}

void gpu_set_framebuffer(gpu_state *gpu, uint32_t width, uint32_t height, pixel_format fmt) {
    free(gpu->fb.pixels);
    gpu->fb.width = width;
    gpu->fb.height = height;
    gpu->fb.format = fmt;

    uint32_t bpp = 4;
    if (fmt == PIX_FMT_RGB565) bpp = 2;
    else if (fmt == PIX_FMT_RGBA16F) bpp = 8;
    else if (fmt == PIX_FMT_R8) bpp = 1;
    else if (fmt == PIX_FMT_RG8) bpp = 2;

    gpu->fb.stride = width * bpp;
    gpu->fb.pixels = calloc(1, gpu->fb.stride * height);
    gpu->fb.dirty = true;
    LOG_INFO("gpu: framebuffer resized to %ux%u", width, height);
}

const uint8_t *gpu_get_framebuffer(gpu_state *gpu) {
    return gpu->fb.pixels;
}

void gpu_clear_framebuffer(gpu_state *gpu, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint8_t *p = gpu->fb.pixels;
    for (uint32_t i = 0; i < gpu->fb.width * gpu->fb.height; i++) {
        p[i * 4 + 0] = r;
        p[i * 4 + 1] = g;
        p[i * 4 + 2] = b;
        p[i * 4 + 3] = a;
    }
    gpu->fb.dirty = true;
}

int gpu_alloc_texture(gpu_state *gpu, uint64_t addr, uint32_t w, uint32_t h, pixel_format fmt) {
    for (int i = 0; i < GPU_MAX_TEXTURES; i++) {
        if (!gpu->textures[i].valid) {
            gpu->textures[i].addr = addr;
            gpu->textures[i].width = w;
            gpu->textures[i].height = h;
            gpu->textures[i].stride = w * 4;
            gpu->textures[i].format = fmt;
            gpu->textures[i].valid = true;
            return i;
        }
    }
    return -1;
}

int gpu_alloc_buffer(gpu_state *gpu, uint64_t addr, uint32_t size) {
    for (int i = 0; i < GPU_MAX_BUFFERS; i++) {
        if (!gpu->buffers[i].valid) {
            gpu->buffers[i].addr = addr;
            gpu->buffers[i].size = size;
            gpu->buffers[i].valid = true;
            return i;
        }
    }
    return -1;
}

void gpu_blit_texture(gpu_state *gpu, int tex_id, int32_t dst_x, int32_t dst_y) {
    if (tex_id < 0 || tex_id >= GPU_MAX_TEXTURES) return;
    gpu_texture *tex = &gpu->textures[tex_id];
    if (!tex->valid) return;

    LOG_TRACE("gpu: blit texture %d (%ux%u) to (%d,%d)",
              tex_id, tex->width, tex->height, dst_x, dst_y);

    /* actual blit would copy from guest memory to framebuffer */
    gpu->fb.dirty = true;
}
