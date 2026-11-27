#define _GNU_SOURCE
#include "fs.h"
#include "../core/log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

void fs_init(fs_state *fs, const char *mount_point) {
    memset(fs, 0, sizeof(*fs));
    strncpy(fs->mount_point, mount_point, FS_PATH_MAX - 1);
    fs->initialized = true;
    LOG_INFO("fs: mounted at %s", fs->mount_point);
}

void fs_destroy(fs_state *fs) {
    for (int i = 0; i < FS_MAX_OPEN; i++) {
        if (fs->handles[i].in_use) {
            free(fs->handles[i].data);
            fs->handles[i].data = NULL;
        }
    }
    fs->initialized = false;
}

void fs_set_save_dir(fs_state *fs, const char *path) {
    strncpy(fs->save_dir, path, FS_PATH_MAX - 1);
}

static void resolve_path(fs_state *fs, const char *path, char *out, size_t out_size) {
    if (path[0] == '/')
        snprintf(out, out_size, "%s%s", fs->mount_point, path);
    else
        snprintf(out, out_size, "%s/%s", fs->mount_point, path);
}

int fs_open(fs_state *fs, const char *path, uint32_t mode) {
    int fd = -1;
    for (int i = 0; i < FS_MAX_OPEN; i++) {
        if (!fs->handles[i].in_use) { fd = i; break; }
    }
    if (fd < 0) return -1;

    char resolved[FS_PATH_MAX];
    resolve_path(fs, path, resolved, sizeof(resolved));

    fs_handle *h = &fs->handles[fd];
    memset(h, 0, sizeof(*h));
    h->in_use = true;
    h->mode = mode;
    strncpy(h->path, resolved, FS_PATH_MAX - 1);

    FILE *f = fopen(resolved, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        h->size = ftell(f);
        fseek(f, 0, SEEK_SET);
        h->capacity = h->size;
        if (h->size > 0) {
            h->data = malloc(h->size);
            fread(h->data, 1, h->size, f);
        }
        fclose(f);
    } else if (mode & FS_MODE_CREATE) {
        h->data = NULL;
        h->size = 0;
        h->capacity = 0;
    } else {
        h->in_use = false;
        return -1;
    }

    if (mode & FS_MODE_APPEND) h->offset = h->size;
    return fd;
}

void fs_close(fs_state *fs, int fd) {
    if (fd < 0 || fd >= FS_MAX_OPEN || !fs->handles[fd].in_use) return;
    fs_handle *h = &fs->handles[fd];

    if (h->mode & (FS_MODE_WRITE | FS_MODE_APPEND | FS_MODE_CREATE)) {
        FILE *f = fopen(h->path, "wb");
        if (f) {
            if (h->data && h->size > 0)
                fwrite(h->data, 1, h->size, f);
            fclose(f);
        }
    }

    free(h->data);
    memset(h, 0, sizeof(*h));
}

int64_t fs_read(fs_state *fs, int fd, void *buf, uint64_t size) {
    if (fd < 0 || fd >= FS_MAX_OPEN || !fs->handles[fd].in_use) return -1;
    fs_handle *h = &fs->handles[fd];
    if (h->offset >= h->size) return 0;

    uint64_t avail = h->size - h->offset;
    uint64_t to_read = size < avail ? size : avail;
    memcpy(buf, h->data + h->offset, to_read);
    h->offset += to_read;
    return (int64_t)to_read;
}

int64_t fs_write(fs_state *fs, int fd, const void *buf, uint64_t size) {
    if (fd < 0 || fd >= FS_MAX_OPEN || !fs->handles[fd].in_use) return -1;
    fs_handle *h = &fs->handles[fd];

    uint64_t needed = h->offset + size;
    if (needed > h->capacity) {
        uint64_t new_cap = needed * 2;
        uint8_t *new_data = realloc(h->data, new_cap);
        if (!new_data) return -1;
        h->data = new_data;
        h->capacity = new_cap;
    }

    memcpy(h->data + h->offset, buf, size);
    h->offset += size;
    if (h->offset > h->size) h->size = h->offset;
    return (int64_t)size;
}

int64_t fs_seek(fs_state *fs, int fd, int64_t offset, fs_whence whence) {
    if (fd < 0 || fd >= FS_MAX_OPEN || !fs->handles[fd].in_use) return -1;
    fs_handle *h = &fs->handles[fd];
    int64_t new_off;

    switch (whence) {
    case FS_SEEK_SET: new_off = offset; break;
    case FS_SEEK_CUR: new_off = (int64_t)h->offset + offset; break;
    case FS_SEEK_END: new_off = (int64_t)h->size + offset; break;
    default: return -1;
    }

    if (new_off < 0) new_off = 0;
    h->offset = (uint64_t)new_off;
    return new_off;
}

uint64_t fs_tell(fs_state *fs, int fd) {
    if (fd < 0 || fd >= FS_MAX_OPEN || !fs->handles[fd].in_use) return 0;
    return fs->handles[fd].offset;
}

uint64_t fs_size(fs_state *fs, int fd) {
    if (fd < 0 || fd >= FS_MAX_OPEN || !fs->handles[fd].in_use) return 0;
    return fs->handles[fd].size;
}

int fs_list_dir(fs_state *fs, const char *path, fs_dir_entry *entries, int max) {
    char resolved[FS_PATH_MAX];
    resolve_path(fs, path, resolved, sizeof(resolved));

    DIR *d = opendir(resolved);
    if (!d) return 0;

    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) && count < max) {
        if (ent->d_name[0] == '.') continue;
        strncpy(entries[count].name, ent->d_name, 255);
        entries[count].type = (ent->d_type == DT_DIR) ? FS_TYPE_DIR : FS_TYPE_FILE;

        char full[FS_PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", resolved, ent->d_name);
        struct stat st;
        if (stat(full, &st) == 0) entries[count].size = st.st_size;
        count++;
    }
    closedir(d);
    return count;
}

bool fs_exists(fs_state *fs, const char *path) {
    char resolved[FS_PATH_MAX];
    resolve_path(fs, path, resolved, sizeof(resolved));
    struct stat st;
    return stat(resolved, &st) == 0;
}

bool fs_mkdir(fs_state *fs, const char *path) {
    char resolved[FS_PATH_MAX];
    resolve_path(fs, path, resolved, sizeof(resolved));
    return mkdir(resolved, 0755) == 0;
}
