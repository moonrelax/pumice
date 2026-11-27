#include "service.h"
#include "../core/log.h"
#include <string.h>

void sm_init(service_manager *sm) {
    memset(sm, 0, sizeof(*sm));
}

bool sm_register(service_manager *sm, const char *name, service_handler handler) {
    if (sm->count >= MAX_SERVICES) return false;

    service_entry *e = &sm->services[sm->count++];
    strncpy(e->name, name, SERVICE_NAME_MAX - 1);
    e->handler = handler;
    e->active = true;

    LOG_INFO("registered service '%s'", name);
    return true;
}

service_entry *sm_get(service_manager *sm, const char *name) {
    for (int i = 0; i < sm->count; i++) {
        if (sm->services[i].active &&
            strncmp(sm->services[i].name, name, SERVICE_NAME_MAX) == 0)
            return &sm->services[i];
    }
    return NULL;
}

void hle_sm_handler(ipc_context *ctx) {
    (void)ctx;
    LOG_TRACE("sm: request");
}

void hle_set_handler(ipc_context *ctx) {
    (void)ctx;
    LOG_TRACE("set: request");
}

void hle_fatal_handler(ipc_context *ctx) {
    (void)ctx;
    LOG_ERR("fatal: service called");
}

void hle_nvdrv_handler(ipc_context *ctx) {
    (void)ctx;
    LOG_TRACE("nvdrv: request");
}

void hle_vi_handler(ipc_context *ctx) {
    (void)ctx;
    LOG_TRACE("vi: request");
}

void hle_hid_handler(ipc_context *ctx) {
    (void)ctx;
    LOG_TRACE("hid: request");
}

void hle_audio_handler(ipc_context *ctx) {
    (void)ctx;
    LOG_TRACE("audio: request");
}

void hle_fs_handler(ipc_context *ctx) {
    (void)ctx;
    LOG_TRACE("fsp-srv: request");
}

void hle_time_handler(ipc_context *ctx) {
    (void)ctx;
    LOG_TRACE("time: request");
}

void hle_register_all(service_manager *sm) {
    sm_register(sm, "sm:",       hle_sm_handler);
    sm_register(sm, "set",       hle_set_handler);
    sm_register(sm, "fatal:u",   hle_fatal_handler);
    sm_register(sm, "nvdrv",     hle_nvdrv_handler);
    sm_register(sm, "nvdrv:a",   hle_nvdrv_handler);
    sm_register(sm, "nvdrv:t",   hle_nvdrv_handler);
    sm_register(sm, "vi:m",      hle_vi_handler);
    sm_register(sm, "vi:s",      hle_vi_handler);
    sm_register(sm, "vi:u",      hle_vi_handler);
    sm_register(sm, "hid",       hle_hid_handler);
    sm_register(sm, "audout:u",  hle_audio_handler);
    sm_register(sm, "audren:u",  hle_audio_handler);
    sm_register(sm, "fsp-srv",   hle_fs_handler);
    sm_register(sm, "time:s",    hle_time_handler);
    sm_register(sm, "time:u",    hle_time_handler);
}
