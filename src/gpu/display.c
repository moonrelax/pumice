#include "display.h"
#include "../core/log.h"
#include <stdio.h>
#include <string.h>

void display_init(display_state *disp, gpu_state *gpu) {
    memset(disp, 0, sizeof(*disp));
    disp->width = GPU_SCREEN_WIDTH;
    disp->height = GPU_SCREEN_HEIGHT;
    disp->vsync_enabled = true;
    disp->gpu = gpu;
    for (int i = 0; i < DISPLAY_MAX_LAYERS; i++) {
        disp->layers[i].alpha = 1.0f;
        disp->layers[i].texture_id = -1;
    }
    LOG_INFO("display: initialized %ux%u", disp->width, disp->height);
}

void display_set_layer(display_state *disp, int layer, display_rect frame, int tex_id) {
    if (layer < 0 || layer >= DISPLAY_MAX_LAYERS) return;
    disp->layers[layer].frame = frame;
    disp->layers[layer].texture_id = tex_id;
    disp->layers[layer].visible = true;
    disp->layers[layer].z_order = layer;
}

void display_set_layer_visibility(display_state *disp, int layer, bool visible) {
    if (layer < 0 || layer >= DISPLAY_MAX_LAYERS) return;
    disp->layers[layer].visible = visible;
}

void display_compose(display_state *disp) {
    gpu_clear_framebuffer(disp->gpu, 0, 0, 0, 255);

    for (int z = 0; z < DISPLAY_MAX_LAYERS; z++) {
        for (int i = 0; i < DISPLAY_MAX_LAYERS; i++) {
            display_layer *l = &disp->layers[i];
            if (!l->visible || l->z_order != z) continue;
            if (l->texture_id >= 0) {
                gpu_blit_texture(disp->gpu, l->texture_id,
                                 l->frame.x, l->frame.y);
            }
        }
    }
}

void display_present(display_state *disp) {
    display_compose(disp);
    disp->frame_count++;
    disp->gpu->fb.dirty = false;
    LOG_TRACE("display: frame %lu", disp->frame_count);
}

void display_dump_ppm(display_state *disp, const char *path) {
    const uint8_t *px = gpu_get_framebuffer(disp->gpu);
    if (!px) return;

    FILE *f = fopen(path, "wb");
    if (!f) {
        LOG_ERR("display: cannot write %s", path);
        return;
    }

    fprintf(f, "P6\n%u %u\n255\n", disp->width, disp->height);
    for (uint32_t i = 0; i < disp->width * disp->height; i++) {
        fputc(px[i * 4 + 0], f);
        fputc(px[i * 4 + 1], f);
        fputc(px[i * 4 + 2], f);
    }
    fclose(f);
    LOG_INFO("display: dumped frame to %s", path);
}

uint64_t display_get_frame_count(display_state *disp) {
    return disp->frame_count;
}
