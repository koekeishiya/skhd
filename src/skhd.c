#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <Carbon/Carbon.h>

#define HASHTABLE_IMPLEMENTATION
#include "hashtable.h"
#include "sbuffer.h"
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

#if 0
#define BEGIN_TIMED_BLOCK() \
    clock_t timed_block_begin = clock()
#define END_TIMED_BLOCK() \
    clock_t timed_block_end = clock(); \
    double timed_block_elapsed = ((timed_block_end - timed_block_begin) / (double)CLOCKS_PER_SEC) * 1000.0f; \
    printf("elapsed time: %.4fms\n", timed_block_elapsed)
#else
#define BEGIN_TIMED_BLOCK()
#define END_TIMED_BLOCK()
#endif

internal unsigned major_version = 0;
internal unsigned minor_version = 2;
internal unsigned patch_version = 1;

internal struct mode *current_mode;
internal struct table mode_map;
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

internal void
warn(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

internal void
parse_config_helper(char *absolutepath)
{
    struct parser parser;
    if (parser_init(&parser, &mode_map, absolutepath)) {
        parse_config(&parser);
        parser_destroy(&parser);
    } else {
        warn("skhd: could not open file '%s'\n", absolutepath);
    }
    current_mode = table_find(&mode_map, "default");
}

internal HOTLOADER_CALLBACK(config_handler)
{
    free_mode_map(&mode_map);
    parse_config_helper(absolutepath);
}

internal EVENT_TAP_CALLBACK(key_handler)
{
    switch (type) {
    case kCGEventTapDisabledByTimeout:
    case kCGEventTapDisabledByUserInput: {
        printf("skhd: restarting event-tap\n");
        struct event_tap *event_tap = (struct event_tap *) reference;
        CGEventTapEnable(event_tap->handle, 1);
    } break;
    case kCGEventKeyDown: {
        if (!current_mode) return event;

        struct hotkey eventkey = create_eventkey(event);
        bool result = find_and_exec_hotkey(&eventkey, &mode_map, &current_mode);
        if (result) return NULL;
    } break;
    case NX_SYSDEFINED: {
        if (!current_mode) return event;

        struct systemkey systemkey = create_systemkey(event);
        if (systemkey.intercept) {
            bool result = find_and_exec_hotkey(&systemkey.eventkey, &mode_map, &current_mode);
            if (result) return NULL;
        }
    } break;
    }
    return event;
}

internal bool
parse_arguments(int argc, char **argv)
{
    int option;
    const char *short_option = "vc:";
    struct option long_option[] = {
        { "version", no_argument, NULL, 'v' },
        { "config", required_argument, NULL, 'c' },
        { NULL, 0, NULL, 0 }
    };

    while ((option = getopt_long(argc, argv, short_option, long_option, NULL)) != -1) {
        switch (option) {
        case 'v': {
            printf("skhd version %d.%d.%d\n", major_version, minor_version, patch_version);
            return true;
        } break;
        case 'c': {
            config_file = strdup(optarg);
        } break;
        }
    }

    return false;
}

internal bool
check_privileges()
{
    bool result;
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
    char *home = getenv("HOME");
    if (home) {
        int length = strlen(home) + strlen("/.skhdrc");
        config_file = (char *) malloc(length + 1);
        strcpy(config_file, home);
        strcat(config_file, "/.skhdrc");
    } else {
        config_file = strdup(".skhdrc");
    }
}

int main(int argc, char **argv)
{
    if (parse_arguments(argc, argv)) {
        return EXIT_SUCCESS;
    }

    if (getuid() == 0 || geteuid() == 0) {
        error("skhd: running as root is not allowed! abort..\n");
    }

    if (secure_keyboard_entry_enabled()) {
        error("skhd: secure keyboard entry is enabled! abort..\n");
    }

    if (!check_privileges()) {
        error("skhd: must be run with accessibility access! abort..\n");
    }

    if (!initialize_keycode_map()) {
        error("skhd: could not initialize keycode map! abort..\n");
    }

    if (!config_file) {
        set_config_path();
    }

    printf("skhd: using config '%s'\n", config_file);
    table_init(&mode_map, 13, (table_hash_func) hash_mode, (table_compare_func) same_mode);
    parse_config_helper(config_file);
    signal(SIGCHLD, SIG_IGN);

    struct event_tap event_tap;
    event_tap.mask = (1 << kCGEventKeyDown) | (1 << NX_SYSDEFINED);
    event_tap_begin(&event_tap, key_handler);

    struct hotloader hotloader = {};
    hotloader_add_file(&hotloader, config_file);
    hotloader_begin(&hotloader, config_handler);

    CFRunLoopRun();
    return EXIT_SUCCESS;
}
