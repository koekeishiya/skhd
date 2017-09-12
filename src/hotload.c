#include "hotload.h"
#include "hotkey.h"

#include <stdlib.h>
#include <string.h>

#define internal static

internal char *
copy_string(const char *s)
{
    unsigned length = strlen(s);
    char *result = malloc(length + 1);
    memcpy(result, s, length);
    result[length] = '\0';
    return result;
}

internal char *
file_directory(const char *file)
{
    char *last_slash = strrchr(file, '/');
    *last_slash = '\0';
    char *directory = copy_string(file);
    *last_slash = '/';
    return directory;
}

internal char *
file_name(const char *file)
{
    char *last_slash = strrchr(file, '/');
    char *name = copy_string(last_slash + 1);
    return name;
}

bool hotloader_watched_file(struct hotloader *hotloader, char *absolutepath)
{
    bool success = false;
    for(unsigned index = 0; success == 0 && index < hotloader->watch_count; ++index) {
        struct watched_file *watch_info = &hotloader->watch_list[index];

        char *directory = file_directory(absolutepath);
        char *filename = file_name(absolutepath);

        if(strcmp(watch_info->directory, directory) == 0) {
            if(strcmp(watch_info->filename, filename) == 0) {
                success = true;
            }
        }

        free(filename);
        free(directory);
    }

    return success;
}

void hotloader_add_file(struct hotloader *hotloader, const char *file)
{
    if(!hotloader->enabled) {
        struct watched_file watch_info;
        watch_info.directory = file_directory(file);
        watch_info.filename = file_name(file);
        hotloader->watch_list[hotloader->watch_count++] = watch_info;
        printf("hotload: watching file '%s' in directory '%s'\n", watch_info.filename, watch_info.directory);
    }
}

bool hotloader_begin(struct hotloader *hotloader, hotloader_callback *callback)
{
    if(!hotloader->enabled) {
        if(hotloader->watch_count) {
            CFStringRef string_refs[hotloader->watch_count];
            for(unsigned index = 0; index < hotloader->watch_count; ++index) {
                string_refs[index] = CFStringCreateWithCString(kCFAllocatorDefault,
                                                               hotloader->watch_list[index].directory,
                                                               kCFStringEncodingUTF8);
            }

            FSEventStreamContext context = {};
            context.info = (void *) hotloader;

            hotloader->enabled = true;
            hotloader->path = (CFArrayRef) CFArrayCreate(NULL, (const void **) string_refs, hotloader->watch_count, &kCFTypeArrayCallBacks);
            hotloader->flags = kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagFileEvents;
            hotloader->stream = FSEventStreamCreate(NULL,
                                                    callback,
                                                    &context,
                                                    hotloader->path,
                                                    kFSEventStreamEventIdSinceNow,
                                                    0.5,
                                                    hotloader->flags);
            FSEventStreamScheduleWithRunLoop(hotloader->stream, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
            FSEventStreamStart(hotloader->stream);
            return true;
        }
    }
    return false;
}

void hotloader_end(struct hotloader *hotloader)
{
    if(hotloader->enabled) {
        FSEventStreamStop(hotloader->stream);
        FSEventStreamInvalidate(hotloader->stream);
        FSEventStreamRelease(hotloader->stream);

        CFIndex count = CFArrayGetCount(hotloader->path);
        for(unsigned index = 0; index < count; ++index) {
            CFStringRef string_ref = (CFStringRef) CFArrayGetValueAtIndex(hotloader->path, index);
            free(hotloader->watch_list[index].directory);
            free(hotloader->watch_list[index].filename);
            CFRelease(string_ref);
        }

        CFRelease(hotloader->path);
        memset(hotloader, 0, sizeof(struct hotloader));
    }
}
