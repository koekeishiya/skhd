#include "hotload.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define internal static

#define FSEVENT_CALLBACK(name) void name(ConstFSEventStreamRef stream,\
                                         void *context,\
                                         size_t count,\
                                         void *paths,\
                                         const FSEventStreamEventFlags *flags,\
                                         const FSEventStreamEventId *ids)

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

internal char *
resolve_symlink(char *file)
{
    struct stat buffer;
    if (lstat(file, &buffer) != 0) {
        return NULL;
    }

    if (!S_ISLNK(buffer.st_mode)) {
        return copy_string(file);
    }

    ssize_t size = buffer.st_size + 1;
    char *result = malloc(size);
    ssize_t read = readlink(file, result, size);

    if (read != -1) {
        result[read] = '\0';
        return result;
    }

    free(result);
    return NULL;
}

internal FSEVENT_CALLBACK(hotloader_handler)
{
    /* NOTE(koekeishiya): We sometimes get two events upon file save. */
    struct hotloader *hotloader = (struct hotloader *) context;
    char **files = (char **) paths;

    for (unsigned file_index = 0; file_index < count; ++file_index) {
        for (unsigned watch_index = 0; watch_index < hotloader->watch_count; ++watch_index) {
            struct watched_file *watch_info = hotloader->watch_list + watch_index;
            if (strcmp(watch_info->absolutepath, files[file_index]) == 0) {
                hotloader->callback(watch_info->absolutepath,
                                    watch_info->directory,
                                    watch_info->filename);
                break;
            }
        }
    }
}

void hotloader_add_file(struct hotloader *hotloader, char *file)
{
    if (!hotloader->enabled) {
        char *real_path = resolve_symlink(file);
        if (real_path) {
            struct watched_file watch_info = {
                .absolutepath = real_path,
                .directory = file_directory(real_path),
                .filename = file_name(real_path)
            };
            hotloader->watch_list[hotloader->watch_count++] = watch_info;
            printf("hotload: watching file '%s' in directory '%s'\n", watch_info.filename, watch_info.directory);
        } else {
            fprintf(stderr, "hotload: could not watch file '%s'\n", file);
        }
    }
}

bool hotloader_begin(struct hotloader *hotloader, hotloader_callback *callback)
{
    if ((hotloader->enabled) ||
        (!hotloader->watch_count)) {
        return false;
    }

    CFStringRef string_refs[hotloader->watch_count];
    for (unsigned index = 0; index < hotloader->watch_count; ++index) {
        string_refs[index] = CFStringCreateWithCString(kCFAllocatorDefault,
                                                       hotloader->watch_list[index].directory,
                                                       kCFStringEncodingUTF8);
    }

    FSEventStreamContext context = {};
    context.info = (void *) hotloader;

    hotloader->enabled = true;
    hotloader->callback = callback;
    hotloader->path = (CFArrayRef) CFArrayCreate(NULL, (const void **) string_refs, hotloader->watch_count, &kCFTypeArrayCallBacks);
    hotloader->flags = kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagFileEvents;
    hotloader->stream = FSEventStreamCreate(NULL,
                                            hotloader_handler,
                                            &context,
                                            hotloader->path,
                                            kFSEventStreamEventIdSinceNow,
                                            0.5,
                                            hotloader->flags);
    FSEventStreamScheduleWithRunLoop(hotloader->stream, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    FSEventStreamStart(hotloader->stream);
    return true;
}

void hotloader_end(struct hotloader *hotloader)
{
    if (hotloader->enabled) {
        FSEventStreamStop(hotloader->stream);
        FSEventStreamInvalidate(hotloader->stream);
        FSEventStreamRelease(hotloader->stream);

        CFIndex count = CFArrayGetCount(hotloader->path);
        for (unsigned index = 0; index < count; ++index) {
            CFStringRef string_ref = (CFStringRef) CFArrayGetValueAtIndex(hotloader->path, index);
            free(hotloader->watch_list[index].absolutepath);
            free(hotloader->watch_list[index].directory);
            free(hotloader->watch_list[index].filename);
            CFRelease(string_ref);
        }

        CFRelease(hotloader->path);
        memset(hotloader, 0, sizeof(struct hotloader));
    }
}
