#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>

#include <Carbon/Carbon.h>

#define HASHTABLE_IMPLEMENTATION
#include "hashtable.h"

#include "hotload.h"
#include "event_tap.h"
#include "locale.h"
#include "tokenize.h"
#include "parse.h"
#include "hotkey.h"

#include "hotload.c"
#include "event_tap.c"
#include "locale.c"
#include "tokenize.c"
#include "parse.c"
#include "hotkey.c"

#define internal static
extern bool CGSIsSecureEventInputSet();
#define secure_keyboard_entry_enabled CGSIsSecureEventInputSet

internal unsigned major_version = 0;
internal unsigned minor_version = 0;
internal unsigned patch_version = 5;
internal struct table hotkey_map;
internal char *config_file;

internal void
error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

internal bool
watched_io_file(struct hotloader *hotloader, char *absolutepath)
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

internal HOTLOADER_CALLBACK(hotloader_handler)
{
    struct hotloader *hotloader = (struct hotloader *) context;

    char **files = (char **) paths;
    for(unsigned index = 0; index < count; ++index) {
        char *absolutepath = files[index];
        if(watched_io_file(hotloader, absolutepath)) {
            /* TODO(koekeishiya): We sometimes get two events upon file save.
             * Filter the duplicated event or something ?? */
            struct parser parser;
            if(parser_init(&parser, absolutepath)) {
                free_hotkeys(&hotkey_map);
                parse_config(&parser, &hotkey_map);
                if(parser.error) {
                    free_hotkeys(&hotkey_map);
                }
                parser_destroy(&parser);
            }
        }
    }
}

#ifdef 0
#define BEGIN_TIMED_BLOCK() \
    clock_t timed_block_begin = clock()
#define END_TIMED_BLOCK() \
    clock_t timed_block_end = clock(); \
    double timed_block_elapsed = (timed_block_end -timed_block_begin) / (double)CLOCKS_PER_SEC; \
    printf("elapsed time: %f\n", timed_block_elapsed)
#endif



internal EVENT_TAP_CALLBACK(key_handler)
{
    switch(type)
    {
        case kCGEventTapDisabledByTimeout:
        case kCGEventTapDisabledByUserInput:
        {
            printf("skhd: restarting event-tap\n");
            struct event_tap *event_tap = (struct event_tap *) reference;
            CGEventTapEnable(event_tap->handle, 1);
        } break;
        case kCGEventKeyDown:
        {
            uint32_t flags = CGEventGetFlags(event);
            uint32_t key = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            struct hotkey eventkey = { .flags = 0, .key = key };
            cgeventflags_to_hotkeyflags(flags, &eventkey);
            bool result = find_and_exec_hotkey(&eventkey, &hotkey_map);
            if(result) {
                return NULL;
            }
        } break;
        default: {} break;
    }

    return event;
}

internal bool
parse_arguments(int argc, char **argv)
{
    int option;
    const char *short_option = "vc:";
    struct option long_option[] =
    {
        { "version", no_argument, NULL, 'v' },
        { "config", required_argument, NULL, 'c' },
        { NULL, 0, NULL, 0 }
    };

    while((option = getopt_long(argc, argv, short_option, long_option, NULL)) != -1) {
        switch(option)
        {
            case 'v':
            {
                printf("skhd version %d.%d.%d\n", major_version, minor_version, patch_version);
                return true;
            } break;
            case 'c':
            {
                config_file = strdup(optarg);
            } break;
        }
    }

    return false;
}

internal bool
check_privileges()
{
    bool result = false;
    const void *keys[] = { kAXTrustedCheckOptionPrompt };
    const void *values[] = { kCFBooleanTrue };

    CFDictionaryRef options;
    options = CFDictionaryCreate(kCFAllocatorDefault,
                                 keys, values, sizeof(keys) / sizeof(*keys),
                                 &kCFCopyStringDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);

    result = AXIsProcessTrustedWithOptions(options);
    CFRelease(options);

    return result;
}

internal void
set_config_path()
{
    if(!config_file) {
        char *home = getenv("HOME");
        if(home) {
            int length = strlen(home) + strlen("/.skhdrc");
            config_file = (char *) malloc(length + 1);
            strcpy(config_file, home);
            strcat(config_file, "/.skhdrc");
        } else {
            config_file = strdup(".skhdrc");
        }
    }
}

int main(int argc, char **argv)
{
    if(parse_arguments(argc, argv)) {
        return EXIT_SUCCESS;
    }

    if(secure_keyboard_entry_enabled()) {
        error("skhd: secure keyboard entry is enabled! abort..\n");
    }

    if(!check_privileges()) {
        error("skhd: must be run with accessibility access.\n");
    }

    signal(SIGCHLD, SIG_IGN);
    set_config_path();
    printf("skhd: using config '%s'\n", config_file);

    table_init(&hotkey_map,
               128,
               (table_hash_func) hash_hotkey,
               (table_compare_func) same_hotkey);

    struct parser parser;
    if(parser_init(&parser, config_file)) {
        parse_config(&parser, &hotkey_map);
        if(parser.error) {
            free_hotkeys(&hotkey_map);
        }
        parser_destroy(&parser);
    } else {
        error("skhd: could not open file '%s'\n", config_file);
    }

    struct event_tap event_tap;
    event_tap.mask = (1 << kCGEventKeyDown);
    event_tap_begin(&event_tap, key_handler);

    struct hotloader hotloader = {};
    hotloader_add_file(&hotloader, config_file);
    hotloader_begin(&hotloader, hotloader_handler);

    CFRunLoopRun();

    return EXIT_SUCCESS;
}
