#include "hotkey.h"
#include "locale.h"
#include <string.h>
#include <pthread.h>

#define internal static
#define local_persist static

#define LRMOD_ALT   0
#define LRMOD_CMD   6
#define LRMOD_CTRL  9
#define LRMOD_SHIFT 3
#define LMOD_OFFS   1
#define RMOD_OFFS   2

internal uint32_t cgevent_lrmod_flag[] =
{
    Event_Mask_Alt,     Event_Mask_LAlt,     Event_Mask_RAlt,
    Event_Mask_Shift,   Event_Mask_LShift,   Event_Mask_RShift,
    Event_Mask_Cmd,     Event_Mask_LCmd,     Event_Mask_RCmd,
    Event_Mask_Control, Event_Mask_LControl, Event_Mask_RControl,
};

internal uint32_t hotkey_lrmod_flag[] =
{
    Hotkey_Flag_Alt,     Hotkey_Flag_LAlt,     Hotkey_Flag_RAlt,
    Hotkey_Flag_Shift,   Hotkey_Flag_LShift,   Hotkey_Flag_RShift,
    Hotkey_Flag_Cmd,     Hotkey_Flag_LCmd,     Hotkey_Flag_RCmd,
    Hotkey_Flag_Control, Hotkey_Flag_LControl, Hotkey_Flag_RControl,
};

internal bool
compare_lr_mod(struct hotkey *a, struct hotkey *b, int mod)
{
    bool result = has_flags(a, hotkey_lrmod_flag[mod])
                ? has_flags(b, hotkey_lrmod_flag[mod + LMOD_OFFS]) ||
                  has_flags(b, hotkey_lrmod_flag[mod + RMOD_OFFS]) ||
                  has_flags(b, hotkey_lrmod_flag[mod])
                : has_flags(a, hotkey_lrmod_flag[mod + LMOD_OFFS]) == has_flags(b, hotkey_lrmod_flag[mod + LMOD_OFFS]) &&
                  has_flags(a, hotkey_lrmod_flag[mod + RMOD_OFFS]) == has_flags(b, hotkey_lrmod_flag[mod + RMOD_OFFS]) &&
                  has_flags(a, hotkey_lrmod_flag[mod])             == has_flags(b, hotkey_lrmod_flag[mod]);
    return result;
}

internal bool
compare_fn(struct hotkey *a, struct hotkey *b)
{
    return has_flags(a, Hotkey_Flag_Fn) == has_flags(b, Hotkey_Flag_Fn);
}

bool same_hotkey(struct hotkey *a, struct hotkey *b)
{
    return compare_lr_mod(a, b, LRMOD_ALT)   &&
           compare_lr_mod(a, b, LRMOD_CMD)   &&
           compare_lr_mod(a, b, LRMOD_CTRL)  &&
           compare_lr_mod(a, b, LRMOD_SHIFT) &&
           compare_fn(a, b) &&
           a->key == b->key;
}

unsigned long hash_hotkey(struct hotkey *a)
{
    return a->key;
}

internal bool
fork_and_exec(char *command)
{
    local_persist char arg[] = "-c";
    local_persist char *shell = NULL;
    if(!shell) {
        char *env_shell = getenv("SHELL");
        shell = env_shell ? env_shell : "/bin/bash";
    }

    int cpid = fork();
    if(cpid == 0) {
        char *exec[] = { shell, arg, command, NULL};
        int status_code = execvp(exec[0], exec);
        exit(status_code);
    }

    return true;
}

bool find_and_exec_hotkey(struct hotkey *eventkey, struct table *hotkey_map)
{
    bool result = false;
    struct hotkey *hotkey;
    if((hotkey = table_find(hotkey_map, eventkey))) {
        if(fork_and_exec(hotkey->command)) {
            result = has_flags(hotkey, Hotkey_Flag_Passthrough) ? false : true;
        }
    }
    return result;
}

void free_hotkeys(struct table *hotkey_map)
{
    int count;
    void **hotkeys = table_reset(hotkey_map, &count);
    for(int index = 0; index < count; ++index) {
        struct hotkey *hotkey = (struct hotkey *) hotkeys[index];
        free(hotkey->command);
        free(hotkey);
    }

    if(count) {
        free(hotkeys);
    }
}

internal void
cgevent_lrmod_flag_to_hotkey_lrmod_flag(CGEventFlags flags, struct hotkey *eventkey, int mod)
{
    enum osx_event_mask mask  = cgevent_lrmod_flag[mod];
    enum osx_event_mask lmask = cgevent_lrmod_flag[mod + LMOD_OFFS];
    enum osx_event_mask rmask = cgevent_lrmod_flag[mod + RMOD_OFFS];

    if((flags & mask) == mask) {
        bool left  = (flags & lmask) == lmask;
        bool right = (flags & rmask) == rmask;

        if(left)            add_flags(eventkey, hotkey_lrmod_flag[mod + LMOD_OFFS]);
        if(right)           add_flags(eventkey, hotkey_lrmod_flag[mod + RMOD_OFFS]);
        if(!left && !right) add_flags(eventkey, hotkey_lrmod_flag[mod]);
    }
}

void cgeventflags_to_hotkeyflags(CGEventFlags flags, struct hotkey *eventkey)
{
    cgevent_lrmod_flag_to_hotkey_lrmod_flag(flags, eventkey, LRMOD_ALT);
    cgevent_lrmod_flag_to_hotkey_lrmod_flag(flags, eventkey, LRMOD_CMD);
    cgevent_lrmod_flag_to_hotkey_lrmod_flag(flags, eventkey, LRMOD_CTRL);
    cgevent_lrmod_flag_to_hotkey_lrmod_flag(flags, eventkey, LRMOD_SHIFT);

    if((flags & Event_Mask_Fn) == Event_Mask_Fn) {
        add_flags(eventkey, Hotkey_Flag_Fn);
    }
}
