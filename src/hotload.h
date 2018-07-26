#ifndef SKHD_HOTLOAD_H
#define SKHD_HOTLOAD_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <Carbon/Carbon.h>

#define HOTLOADER_CALLBACK(name) void name(char *absolutepath, char *directory, char *filename)
typedef HOTLOADER_CALLBACK(hotloader_callback);

struct watched_entry;
struct hotloader
{
    FSEventStreamEventFlags flags;
    FSEventStreamRef stream;
    CFArrayRef path;
    bool enabled;

    hotloader_callback *callback;
    struct watched_entry *watch_list;
    unsigned watch_capacity;
    unsigned watch_count;
};

bool hotloader_begin(struct hotloader *hotloader, hotloader_callback *callback);
void hotloader_end(struct hotloader *hotloader);
bool hotloader_add_catalog(struct hotloader *hotloader, const char *directory, const char *extension);
bool hotloader_add_file(struct hotloader *hotloader, const char *file);

#endif
