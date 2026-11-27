#include "hid.h"
#include "../core/log.h"
#include <string.h>

void hid_init(hid_state *hid) {
    memset(hid, 0, sizeof(*hid));
    hid_connect_controller(hid, 0, CONTROLLER_HANDHELD);
    LOG_INFO("hid: initialized with handheld controller");
}

void hid_connect_controller(hid_state *hid, int player, controller_type type) {
    if (player < 0 || player >= HID_MAX_CONTROLLERS) return;
    hid->controllers[player].type = type;
    hid->controllers[player].connected = true;
    hid->controllers[player].buttons = 0;
    hid->controllers[player].left_stick = (hid_stick){0, 0};
    hid->controllers[player].right_stick = (hid_stick){0, 0};
    LOG_INFO("hid: player %d connected (type %d)", player, type);
}

void hid_disconnect_controller(hid_state *hid, int player) {
    if (player < 0 || player >= HID_MAX_CONTROLLERS) return;
    memset(&hid->controllers[player], 0, sizeof(hid_controller));
}

void hid_set_buttons(hid_state *hid, int player, uint32_t buttons) {
    if (player < 0 || player >= HID_MAX_CONTROLLERS) return;
    hid->controllers[player].buttons = buttons;
}

void hid_set_stick(hid_state *hid, int player, bool right, int16_t x, int16_t y) {
    if (player < 0 || player >= HID_MAX_CONTROLLERS) return;
    hid_stick *s = right ? &hid->controllers[player].right_stick
                         : &hid->controllers[player].left_stick;
    s->x = x;
    s->y = y;
}

void hid_set_touch(hid_state *hid, int idx, int32_t x, int32_t y, bool active) {
    if (idx < 0 || idx >= HID_MAX_TOUCHES) return;
    hid->touches[idx].x = x;
    hid->touches[idx].y = y;
    hid->touches[idx].active = active;
    if (active && (uint32_t)idx >= hid->touch_count)
        hid->touch_count = idx + 1;
}

void hid_set_sixaxis(hid_state *hid, int player, float ax, float ay, float az,
                     float gx, float gy, float gz) {
    if (player < 0 || player >= HID_MAX_CONTROLLERS) return;
    hid->sixaxis[player].accel[0] = ax;
    hid->sixaxis[player].accel[1] = ay;
    hid->sixaxis[player].accel[2] = az;
    hid->sixaxis[player].gyro[0] = gx;
    hid->sixaxis[player].gyro[1] = gy;
    hid->sixaxis[player].gyro[2] = gz;
    hid->sixaxis[player].enabled = true;
}

void hid_update(hid_state *hid) {
    hid->timestamp++;
}

uint32_t hid_get_buttons(hid_state *hid, int player) {
    if (player < 0 || player >= HID_MAX_CONTROLLERS) return 0;
    return hid->controllers[player].buttons;
}

hid_stick hid_get_stick(hid_state *hid, int player, bool right) {
    if (player < 0 || player >= HID_MAX_CONTROLLERS) return (hid_stick){0, 0};
    return right ? hid->controllers[player].right_stick
                 : hid->controllers[player].left_stick;
}

uint32_t hid_get_touch_count(hid_state *hid) {
    return hid->touch_count;
}
