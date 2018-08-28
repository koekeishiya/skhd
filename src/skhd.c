#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <Carbon/Carbon.h>

#include "log.h"
#define HASHTABLE_IMPLEMENTATION
#include "hashtable.h"
#include "sbuffer.h"
#include "hotload.h"
#include "event_tap.h"
#include "locale.h"
#include "tokenize.h"
#include "parse.h"
#include "hotkey.h"
#include "synthesize.h"

#include "hotload.c"
#include "event_tap.c"
#include "locale.c"
#include "tokenize.c"
#include "parse.c"
#include "hotkey.c"
#include "synthesize.c"
// #include "carbon.c"

#define internal static
extern bool CGSIsSecureEventInputSet();
#define secure_keyboard_entry_enabled CGSIsSecureEventInputSet

#ifdef SKHD_PROFILE
#define BEGIN_SCOPED_TIMED_BLOCK(note) \
    do { \
        char *timed_note = note; \
        clock_t timed_block_begin = clock()
#define END_SCOPED_TIMED_BLOCK() \
        clock_t timed_block_end = clock(); \
        double timed_block_elapsed = ((timed_block_end - timed_block_begin) / (double)CLOCKS_PER_SEC) * 1000.0f; \
        printf("%.4fms (%s)\n", timed_block_elapsed, timed_note); \
    } while (0)
#define BEGIN_TIMED_BLOCK(note) \
        char *timed_note = note; \
        clock_t timed_block_begin = clock()
#define END_TIMED_BLOCK() \
        clock_t timed_block_end = clock(); \
        double timed_block_elapsed = ((timed_block_end - timed_block_begin) / (double)CLOCKS_PER_SEC) * 1000.0f; \
        printf("%.4fms (%s)\n", timed_block_elapsed, timed_note)
#else
#define BEGIN_SCOPED_TIMED_BLOCK(note)
#define END_SCOPED_TIMED_BLOCK()
#define BEGIN_TIMED_BLOCK(note)
#define END_TIMED_BLOCK()
#endif

#define SKHD_CONFIG_FILE ".skhdrc"

internal unsigned major_version = 0;
internal unsigned minor_version = 2;
internal unsigned patch_version = 5;

// internal struct carbon_event carbon;
internal struct event_tap event_tap;
internal struct hotloader hotloader;
internal struct mode *current_mode;
internal struct table mode_map;
internal char *config_file;

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
    BEGIN_TIMED_BLOCK("hotload_config");
    free_mode_map(&mode_map);
    parse_config_helper(absolutepath);
    END_TIMED_BLOCK();
}

internal EVENT_TAP_CALLBACK(key_handler)
{
    switch (type) {
    case kCGEventTapDisabledByTimeout:
    case kCGEventTapDisabledByUserInput: {
        debug("skhd: restarting event-tap\n");
        struct event_tap *event_tap = (struct event_tap *) reference;
        CGEventTapEnable(event_tap->handle, 1);
    } break;
    case kCGEventKeyDown: {
        if (!current_mode) return event;

        BEGIN_TIMED_BLOCK("handle_keypress");
        struct hotkey eventkey = create_eventkey(event);
        bool result = find_and_exec_hotkey(&eventkey, &mode_map, &current_mode);
        END_TIMED_BLOCK();

        if (result) return NULL;
    } break;
    case NX_SYSDEFINED: {
        if (!current_mode) return event;

        struct hotkey eventkey;
        if (intercept_systemkey(event, &eventkey)) {
            bool result = find_and_exec_hotkey(&eventkey, &mode_map, &current_mode);
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
    const char *short_option = "Vvc:k:t:";
    struct option long_option[] = {
        { "verbose", no_argument, NULL, 'V' },
        { "version", no_argument, NULL, 'v' },
        { "config", required_argument, NULL, 'c' },
        { "key", required_argument, NULL, 'k' },
        { "text", required_argument, NULL, 't' },
        { NULL, 0, NULL, 0 }
    };

    while ((option = getopt_long(argc, argv, short_option, long_option, NULL)) != -1) {
        switch (option) {
        case 'V': {
            verbose = true;
        } break;
        case 'v': {
            printf("skhd version %d.%d.%d\n", major_version, minor_version, patch_version);
            return true;
        } break;
        case 'c': {
            config_file = copy_string(optarg);
        } break;
        case 'k': {
            synthesize_key(optarg);
            return true;
        } break;
        case 't': {
            synthesize_text(optarg);
            return true;
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
use_default_config_path()
{
    char *home = getenv("HOME");
    if (!home) {
        error("skhd: could not locate config because 'env HOME' was not set! abort..\n");
    }

    int home_len = strlen(home);
    int config_len = strlen("/"SKHD_CONFIG_FILE);
    int length = home_len + config_len;
    config_file = malloc(length + 1);
    memcpy(config_file, home, home_len);
    memcpy(config_file + home_len, "/"SKHD_CONFIG_FILE, config_len);
    config_file[length] = '\0';
}

int main(int argc, char **argv)
{
    BEGIN_TIMED_BLOCK("startup");
    BEGIN_SCOPED_TIMED_BLOCK("initialization");
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

    /*
     * NOTE(koekeishiya: hooks up event for tracking name of focused process/application
     *
     *  if (!carbon_event_init(&carbon)) {
     *      error("skhd: could not initialize carbon events! abort..\n");
     *  }
     */

    if (!config_file) {
        use_default_config_path();
    }

    signal(SIGCHLD, SIG_IGN);
    init_shell();
    table_init(&mode_map, 13, (table_hash_func) hash_mode, (table_compare_func) same_mode);
    END_SCOPED_TIMED_BLOCK();

    BEGIN_SCOPED_TIMED_BLOCK("parse_config");
    debug("skhd: using config '%s'\n", config_file);
    parse_config_helper(config_file);
    END_SCOPED_TIMED_BLOCK();

    BEGIN_SCOPED_TIMED_BLOCK("begin_eventtap");
    event_tap.mask = (1 << kCGEventKeyDown) | (1 << NX_SYSDEFINED);
    event_tap_begin(&event_tap, key_handler);
    END_SCOPED_TIMED_BLOCK();

    BEGIN_SCOPED_TIMED_BLOCK("begin_hotloader");
    if (hotloader_add_file(&hotloader, config_file) &&
        hotloader_begin(&hotloader, config_handler)) {
        debug("skhd: watching '%s' for changes\n", config_file);
    } else {
        warn("skhd: could not watch '%s'\n", config_file);
    }
    END_SCOPED_TIMED_BLOCK();
    END_TIMED_BLOCK();

    CFRunLoopRun();
    return EXIT_SUCCESS;
}
