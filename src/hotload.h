#ifndef SKHD_HOTLOAD_H
#define SKHD_HOTLOAD_H

#include <stdbool.h>
#include <Carbon/Carbon.h>

#define HOTLOADER_CALLBACK(name) void name(ConstFSEventStreamRef stream,\
                                           void *context,\
                                           size_t count,\
                                           void *paths,\
                                           const FSEventStreamEventFlags *flags,\
                                           const FSEventStreamEventId *ids)
typedef HOTLOADER_CALLBACK(hotloader_callback);

struct watched_file
{
    char *directory;
    char *filename;
};

struct hotloader
{
    FSEventStreamEventFlags flags;
    FSEventStreamRef stream;
    CFArrayRef path;
    bool enabled;

    struct watched_file watch_list[32];
    unsigned watch_count;
};

bool hotloader_begin(struct hotloader *hotloader, hotloader_callback *callback);
void hotloader_end(struct hotloader *hotloader);

void hotloader_add_file(struct hotloader *hotloader, const char *file);
bool hotloader_watched_file(struct hotloader *hotloader, char *absolutepath);

#endif
