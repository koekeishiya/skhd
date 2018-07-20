#ifndef SKHD_HOTLOAD_H
#define SKHD_HOTLOAD_H

#include <stdbool.h>
#include <Carbon/Carbon.h>

#define HOTLOADER_CALLBACK(name) void name(char *absolutepath, char *directory, char *filename)
typedef HOTLOADER_CALLBACK(hotloader_callback);

enum watch_kind
{
    WATCH_KIND_INVALID,
    WATCH_KIND_CATALOG,
    WATCH_KIND_FILE
};

struct watched_catalog
{
    char *directory;
    char *extension;
}

struct watched_file
{
    char *absolutepath;
    char *directory;
    char *filename;
};

struct watched_entry
{
    enum watch_kind kind;
    union {
        struct watched_file file_info;
        struct watched_catalog catalog_info;
    };
};

struct hotloader
{
    FSEventStreamEventFlags flags;
    FSEventStreamRef stream;
    CFArrayRef path;
    bool enabled;

    hotloader_callback *callback;
    struct watched_entry watch_list[32];
    unsigned watch_count;
};

bool hotloader_begin(struct hotloader *hotloader, hotloader_callback *callback);
void hotloader_end(struct hotloader *hotloader);
void hotloader_add_catalog(struct hotloader *hotloader, char *directory, char *extension);
void hotloader_add_file(struct hotloader *hotloader, char *file);

#endif
