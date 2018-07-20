#include "hotload.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define internal static

#define FSEVENT_CALLBACK(name) void name(ConstFSEventStreamRef stream,\
                                         void *context,\
                                         size_t file_count,\
                                         void *file_paths,\
                                         const FSEventStreamEventFlags *flags,\
                                         const FSEventStreamEventId *ids)

internal inline bool
same_string(const char *a, const char *b)
{
    bool result = a && b && strcmp(a, b) == 0;
    return result;
}

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
resolve_symlink(const char *file)
{
    struct stat buffer;
    if (lstat(file, &buffer) != 0) {
        return NULL;
    }

    if (S_ISDIR(buffer.st_mode)) {
        return copy_string(file);
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

internal enum watch_kind
resolve_watch_kind(char *file)
{
    struct stat buffer;
    if (lstat(file, &buffer) != 0) {
        return WATCH_KIND_INVALID;
    }

    if (S_ISDIR(buffer.st_mode)) {
        return WATCH_KIND_CATALOG;
    }

    if (!S_ISLNK(buffer.st_mode)) {
        return WATCH_KIND_FILE;
    }

    return WATCH_KIND_INVALID;
}

internal char *
same_catalog(char *absolutepath, struct watched_catalog *catalog_info)
{
    char *last_slash = strrchr(absolutepath, '/');
    if (!last_slash) return NULL;

    char *filename = NULL;

    // NOTE(koekeisihya): null terminate '/' to cut off filename
    *last_slash = '\0';

    if (same_string(absolutepath, catalog_info->directory)) {
        filename = !catalog_info->extension
                 ? last_slash + 1
                 : same_string(catalog_info->extension, strrchr(last_slash + 1, '.'))
                 ? last_slash + 1
                 : NULL;
    }

    // NOTE(koekeisihya): revert '/' to restore filename
    *last_slash = '/';

    return filename;
}

internal inline bool
same_file(char *absolutepath, struct watched_file *file_info)
{
    bool result = same_string(absolutepath, file_info->absolutepath);
    return result;
}

internal FSEVENT_CALLBACK(hotloader_handler)
{
    /* NOTE(koekeishiya): We sometimes get two events upon file save. */
    struct hotloader *hotloader = (struct hotloader *) context;
    char **files = (char **) file_paths;

    for (unsigned file_index = 0; file_index < file_count; ++file_index) {
        for (unsigned watch_index = 0; watch_index < hotloader->watch_count; ++watch_index) {
            struct watched_entry *watch_info = hotloader->watch_list + watch_index;
            if (watch_info->kind == WATCH_KIND_CATALOG) {
                char *filename = same_catalog(files[file_index], &watch_info->catalog_info);
                if (!filename) continue;

                hotloader->callback(files[file_index],
                                    watch_info->catalog_info.directory,
                                    filename);
                break;
            } else if (watch_info->kind == WATCH_KIND_FILE) {
                bool match = same_file(files[file_index], &watch_info->file_info);
                if (!match) continue;

                hotloader->callback(watch_info->file_info.absolutepath,
                                    watch_info->file_info.directory,
                                    watch_info->file_info.filename);
                break;
            }
        }
    }
}

bool hotloader_add_catalog(struct hotloader *hotloader, const char *directory, const char *extension)
{
    if (hotloader->enabled) return false;

    char *real_path = resolve_symlink(directory);
    if (!real_path) return false;

    enum watch_kind kind = resolve_watch_kind(real_path);
    if (kind != WATCH_KIND_CATALOG) return false;

    struct watched_entry entry = {
        .kind = WATCH_KIND_CATALOG,
        .catalog_info = {
            .directory = real_path,
            .extension = extension
                       ? copy_string(extension)
                       : NULL
        }
    };
    hotloader->watch_list[hotloader->watch_count++] = entry;

    return true;
}

bool hotloader_add_file(struct hotloader *hotloader, const char *file)
{
    if (hotloader->enabled) return false;

    char *real_path = resolve_symlink(file);
    if (!real_path) return false;

    enum watch_kind kind = resolve_watch_kind(real_path);
    if (kind != WATCH_KIND_FILE) return false;

    struct watched_entry entry = {
        .kind = WATCH_KIND_FILE,
        .file_info = {
            .absolutepath = real_path,
            .directory = file_directory(real_path),
            .filename = file_name(real_path)
        }
    };
    hotloader->watch_list[hotloader->watch_count++] = entry;

    return true;
}

bool hotloader_begin(struct hotloader *hotloader, hotloader_callback *callback)
{
    if (hotloader->enabled || !hotloader->watch_count) return false;

    CFStringRef string_refs[hotloader->watch_count];
    for (unsigned index = 0; index < hotloader->watch_count; ++index) {
        struct watched_entry *watch_info = hotloader->watch_list + index;
        char *directory = watch_info->kind == WATCH_KIND_FILE
                        ? watch_info->file_info.directory
                        : watch_info->catalog_info.directory;
        string_refs[index] = CFStringCreateWithCString(kCFAllocatorDefault,
                                                       directory,
                                                       kCFStringEncodingUTF8);
    }

    FSEventStreamContext context = {
        .info = hotloader
    };

    hotloader->path = (CFArrayRef) CFArrayCreate(NULL,
                                                 (const void **) string_refs,
                                                 hotloader->watch_count,
                                                 &kCFTypeArrayCallBacks);

    hotloader->flags = kFSEventStreamCreateFlagNoDefer |
                       kFSEventStreamCreateFlagFileEvents;

    hotloader->stream = FSEventStreamCreate(NULL,
                                            hotloader_handler,
                                            &context,
                                            hotloader->path,
                                            kFSEventStreamEventIdSinceNow,
                                            0.5,
                                            hotloader->flags);

    FSEventStreamScheduleWithRunLoop(hotloader->stream,
                                     CFRunLoopGetMain(),
                                     kCFRunLoopDefaultMode);

    hotloader->callback = callback;
    FSEventStreamStart(hotloader->stream);
    hotloader->enabled = true;

    return true;
}

void hotloader_end(struct hotloader *hotloader)
{
    if (!hotloader->enabled) return;

    FSEventStreamStop(hotloader->stream);
    FSEventStreamInvalidate(hotloader->stream);
    FSEventStreamRelease(hotloader->stream);

    CFIndex count = CFArrayGetCount(hotloader->path);
    for (unsigned index = 0; index < count; ++index) {
        struct watched_entry *watch_info = hotloader->watch_list + index;
        if (watch_info->kind == WATCH_KIND_FILE) {
            free(watch_info->file_info.absolutepath);
            free(watch_info->file_info.directory);
            free(watch_info->file_info.filename);
        } else if (watch_info->kind == WATCH_KIND_CATALOG) {
            free(watch_info->catalog_info.directory);
            if (watch_info->catalog_info.extension) {
                free(watch_info->catalog_info.extension);
            }
        }
        CFRelease(CFArrayGetValueAtIndex(hotloader->path, index));
    }

    CFRelease(hotloader->path);
    memset(hotloader, 0, sizeof(struct hotloader));
}
