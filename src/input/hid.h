#ifndef PUMICE_HID_H
#define PUMICE_HID_H

#include <stdint.h>
#include <stdbool.h>

#define HID_MAX_CONTROLLERS 8
#define HID_MAX_TOUCHES     16

typedef enum {
    BTN_A        = (1 << 0),
    BTN_B        = (1 << 1),
    BTN_X        = (1 << 2),
    BTN_Y        = (1 << 3),
    BTN_L        = (1 << 4),
    BTN_R        = (1 << 5),
    BTN_ZL       = (1 << 6),
    BTN_ZR       = (1 << 7),
    BTN_PLUS     = (1 << 8),
    BTN_MINUS    = (1 << 9),
    BTN_DLEFT    = (1 << 10),
    BTN_DUP      = (1 << 11),
    BTN_DRIGHT   = (1 << 12),
    BTN_DDOWN    = (1 << 13),
    BTN_LSTICK   = (1 << 14),
    BTN_RSTICK   = (1 << 15),
    BTN_HOME     = (1 << 16),
    BTN_CAPTURE  = (1 << 17),
} hid_button;

typedef enum {
    CONTROLLER_NONE,
    CONTROLLER_HANDHELD,
    CONTROLLER_JOYCON_L,
    CONTROLLER_JOYCON_R,
    CONTROLLER_JOYCON_DUAL,
    CONTROLLER_PRO,
} controller_type;

typedef struct {
    int16_t x;
    int16_t y;
} hid_stick;

typedef struct {
    uint32_t buttons;
    hid_stick left_stick;
    hid_stick right_stick;
    controller_type type;
    bool connected;
} hid_controller;

typedef struct {
    int32_t x, y;
    int32_t dx, dy;
    uint32_t diameter;
    uint32_t angle;
    bool active;
} hid_touch_point;

typedef struct {
    float accel[3];
    float gyro[3];
    bool enabled;
} hid_sixaxis;

typedef struct {
    hid_controller controllers[HID_MAX_CONTROLLERS];
    hid_touch_point touches[HID_MAX_TOUCHES];
    uint32_t touch_count;
    hid_sixaxis sixaxis[HID_MAX_CONTROLLERS];
    uint64_t timestamp;
} hid_state;

void hid_init(hid_state *hid);
void hid_connect_controller(hid_state *hid, int player, controller_type type);
void hid_disconnect_controller(hid_state *hid, int player);

void hid_set_buttons(hid_state *hid, int player, uint32_t buttons);
void hid_set_stick(hid_state *hid, int player, bool right, int16_t x, int16_t y);
void hid_set_touch(hid_state *hid, int idx, int32_t x, int32_t y, bool active);
void hid_set_sixaxis(hid_state *hid, int player, float ax, float ay, float az,
                     float gx, float gy, float gz);
void hid_update(hid_state *hid);

uint32_t hid_get_buttons(hid_state *hid, int player);
hid_stick hid_get_stick(hid_state *hid, int player, bool right);
uint32_t hid_get_touch_count(hid_state *hid);

#endif
