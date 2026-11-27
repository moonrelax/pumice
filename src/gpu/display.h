#ifndef PUMICE_DISPLAY_H
#define PUMICE_DISPLAY_H

#include "gpu.h"

#define DISPLAY_MAX_LAYERS 8

typedef enum {
    TRANSFORM_NONE,
    TRANSFORM_FLIP_H,
    TRANSFORM_FLIP_V,
    TRANSFORM_ROT_90,
    TRANSFORM_ROT_180,
    TRANSFORM_ROT_270,
} display_transform;

typedef struct {
    int32_t x, y;
    uint32_t width, height;
} display_rect;

typedef struct {
    bool visible;
    int32_t z_order;
    display_rect crop;
    display_rect frame;
    display_transform transform;
    float alpha;
    int texture_id;
} display_layer;

typedef struct {
    uint32_t width;
    uint32_t height;
    bool vsync_enabled;
    uint64_t frame_count;
    display_layer layers[DISPLAY_MAX_LAYERS];
    gpu_state *gpu;
} display_state;

void display_init(display_state *disp, gpu_state *gpu);
void display_set_layer(display_state *disp, int layer, display_rect frame, int tex_id);
void display_set_layer_visibility(display_state *disp, int layer, bool visible);
void display_compose(display_state *disp);
void display_present(display_state *disp);
void display_dump_ppm(display_state *disp, const char *path);
uint64_t display_get_frame_count(display_state *disp);

#endif
