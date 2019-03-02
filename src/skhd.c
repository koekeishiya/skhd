#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <Carbon/Carbon.h>

#include "timing.h"
#include "log.h"
#define HASHTABLE_IMPLEMENTATION
#include "hashtable.h"
#include "sbuffer.h"
#include "hotload.h"
#include "event_tap.h"
#include "locale.h"
#include "carbon.h"
#include "tokenize.h"
#include "parse.h"
#include "hotkey.h"
#include "synthesize.h"

#include "hotload.c"
#include "event_tap.c"
#include "locale.c"
#include "carbon.c"
#include "tokenize.c"
#include "parse.c"
#include "hotkey.c"
#include "synthesize.c"

extern bool CGSIsSecureEventInputSet();
#define secure_keyboard_entry_enabled CGSIsSecureEventInputSet

#define internal static
#define global   static

#define SKHD_CONFIG_FILE ".skhdrc"

global unsigned major_version = 0;
global unsigned minor_version = 3;
global unsigned patch_version = 1;

global struct carbon_event carbon;
global struct event_tap event_tap;
global struct hotloader hotloader;
global struct mode *current_mode;
global struct table mode_map;
global char *config_file;

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
    debug("skhd: config-file has been modified.. reloading config\n");
    free_mode_map(&mode_map);
    parse_config_helper(absolutepath);
    END_TIMED_BLOCK();
}

internal CF_NOTIFICATION_CALLBACK(keymap_handler)
{
    BEGIN_TIMED_BLOCK("keymap_changed");
    if (initialize_keycode_map()) {
        debug("skhd: input source changed.. reloading config\n");
        free_mode_map(&mode_map);
        parse_config_helper(config_file);
    }
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
        bool result = find_and_exec_hotkey(&eventkey, &mode_map, &current_mode, &carbon);
        END_TIMED_BLOCK();

        if (result) return NULL;
    } break;
    case NX_SYSDEFINED: {
        if (!current_mode) return event;

        struct hotkey eventkey;
        if (intercept_systemkey(event, &eventkey)) {
            bool result = find_and_exec_hotkey(&eventkey, &mode_map, &current_mode, &carbon);
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
    const char *short_option = "VPvc:k:t:";
    struct option long_option[] = {
        { "verbose", no_argument, NULL, 'V' },
        { "profile", no_argument, NULL, 'P' },
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
        case 'P': {
            profile = true;
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
check_privileges(void)
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
use_default_config_path(void)
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
    if (parse_arguments(argc, argv)) {
        return EXIT_SUCCESS;
    }

    BEGIN_SCOPED_TIMED_BLOCK("total_time");
    BEGIN_SCOPED_TIMED_BLOCK("init");
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

    if (!carbon_event_init(&carbon)) {
        error("skhd: could not initialize carbon events! abort..\n");
    }

    if (!config_file) {
        use_default_config_path();
    }

    CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                    NULL,
                                    &keymap_handler,
                                    kTISNotifySelectedKeyboardInputSourceChanged,
                                    NULL,
                                    CFNotificationSuspensionBehaviorCoalesce);

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
    END_SCOPED_TIMED_BLOCK();

    CFRunLoopRun();
    return EXIT_SUCCESS;
}
