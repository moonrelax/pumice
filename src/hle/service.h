#ifndef PUMICE_SERVICE_H
#define PUMICE_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SERVICES 64
#define SERVICE_NAME_MAX 12

typedef struct ipc_context {
    uint32_t *cmd_buf;
    uint32_t  handle;
} ipc_context;

typedef void (*service_handler)(ipc_context *ctx);

typedef struct {
    char            name[SERVICE_NAME_MAX];
    service_handler handler;
    bool            active;
} service_entry;

typedef struct {
    service_entry services[MAX_SERVICES];
    int           count;
} service_manager;

void  sm_init(service_manager *sm);
bool  sm_register(service_manager *sm, const char *name, service_handler handler);
service_entry *sm_get(service_manager *sm, const char *name);

void hle_sm_handler(ipc_context *ctx);
void hle_set_handler(ipc_context *ctx);
void hle_fatal_handler(ipc_context *ctx);
void hle_nvdrv_handler(ipc_context *ctx);
void hle_vi_handler(ipc_context *ctx);
void hle_hid_handler(ipc_context *ctx);
void hle_audio_handler(ipc_context *ctx);
void hle_fs_handler(ipc_context *ctx);
void hle_time_handler(ipc_context *ctx);

void hle_register_all(service_manager *sm);

#endif
