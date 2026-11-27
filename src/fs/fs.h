#ifndef PUMICE_FS_H
#define PUMICE_FS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define FS_MAX_OPEN   64
#define FS_PATH_MAX   512
#define FS_MAX_ENTRIES 128

typedef enum {
    FS_MODE_READ   = (1 << 0),
    FS_MODE_WRITE  = (1 << 1),
    FS_MODE_APPEND = (1 << 2),
    FS_MODE_CREATE = (1 << 3),
} fs_mode;

typedef enum {
    FS_SEEK_SET,
    FS_SEEK_CUR,
    FS_SEEK_END,
} fs_whence;

typedef enum {
    FS_TYPE_FILE,
    FS_TYPE_DIR,
    FS_TYPE_STORAGE,
} fs_entry_type;

typedef struct {
    char name[256];
    fs_entry_type type;
    uint64_t size;
} fs_dir_entry;

typedef struct {
    bool in_use;
    char path[FS_PATH_MAX];
    uint8_t *data;
    uint64_t size;
    uint64_t capacity;
    uint64_t offset;
    uint32_t mode;
} fs_handle;

typedef struct {
    fs_handle handles[FS_MAX_OPEN];
    char mount_point[FS_PATH_MAX];
    char save_dir[FS_PATH_MAX];
    bool initialized;
} fs_state;

void fs_init(fs_state *fs, const char *mount_point);
void fs_destroy(fs_state *fs);
void fs_set_save_dir(fs_state *fs, const char *path);

int  fs_open(fs_state *fs, const char *path, uint32_t mode);
void fs_close(fs_state *fs, int fd);
int64_t fs_read(fs_state *fs, int fd, void *buf, uint64_t size);
int64_t fs_write(fs_state *fs, int fd, const void *buf, uint64_t size);
int64_t fs_seek(fs_state *fs, int fd, int64_t offset, fs_whence whence);
uint64_t fs_tell(fs_state *fs, int fd);
uint64_t fs_size(fs_state *fs, int fd);

int fs_list_dir(fs_state *fs, const char *path, fs_dir_entry *entries, int max);
bool fs_exists(fs_state *fs, const char *path);
bool fs_mkdir(fs_state *fs, const char *path);

#endif
