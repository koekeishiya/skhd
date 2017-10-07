#include "hotkey.h"
#include <stdlib.h>

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

bool same_mode(char *a, char *b)
{
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

unsigned long hash_mode(char *key)
{
    unsigned long hash = 0, high;
    while(*key) {
        hash = (hash << 4) + *key++;
        high = hash & 0xF0000000;
        if(high) {
            hash ^= (high >> 24);
        }
        hash &= ~high;
    }
    return hash;
}

internal bool
fork_and_exec(char *command)
{
    local_persist char arg[] = "-c";
    local_persist char *shell = NULL;
    if (!shell) {
        char *env_shell = getenv("SHELL");
        shell = env_shell ? env_shell : "/bin/bash";
    }

    int cpid = fork();
    if (cpid == 0) {
        char *exec[] = { shell, arg, command, NULL};
        int status_code = execvp(exec[0], exec);
        exit(status_code);
    }

    return true;
}

bool find_and_exec_hotkey(struct hotkey *eventkey, struct table *mode_map, struct mode **current_mode)
{
    bool result = false;
    struct hotkey *hotkey;
    if ((hotkey = table_find(&(*current_mode)->hotkey_map, eventkey))) {
        if (has_flags(hotkey, Hotkey_Flag_Activate)) {
            *current_mode = table_find(mode_map, hotkey->command);
            if ((*current_mode)->command) {
                fork_and_exec((*current_mode)->command);
            }
            result = has_flags(hotkey, Hotkey_Flag_Passthrough) ? false : true;
        } else if (fork_and_exec(hotkey->command)) {
            result = has_flags(hotkey, Hotkey_Flag_Passthrough) ? false : true;
        }
    }
    return result;
}

internal void
free_hotkeys(struct table *hotkey_map)
{
    int count;
    void **hotkeys = table_reset(hotkey_map, &count);
    for (int index = 0; index < count; ++index) {
        struct hotkey *hotkey = (struct hotkey *) hotkeys[index];
        // the same hotkey can be added for multiple modes
        // we need to know if this pointer was already freed
        // by a mode that was destroyed before us
        // free(hotkey->command);
        // free(hotkey);
    }

    if (count) {
        free(hotkeys);
    }
}

void free_mode_map(struct table *mode_map)
{
    int count;
    void **modes = table_reset(mode_map, &count);
    for (int index = 0; index < count; ++index) {
        struct mode *mode = (struct mode *) modes[index];
        if (mode->command) free(mode->command);
        if (mode->name) free(mode->name);
        free_hotkeys(&mode->hotkey_map);
        free(mode);
    }

    if (count) {
        free(modes);
    }
}

internal void
cgevent_lrmod_flag_to_hotkey_lrmod_flag(CGEventFlags flags, struct hotkey *eventkey, int mod)
{
    enum osx_event_mask mask  = cgevent_lrmod_flag[mod];
    enum osx_event_mask lmask = cgevent_lrmod_flag[mod + LMOD_OFFS];
    enum osx_event_mask rmask = cgevent_lrmod_flag[mod + RMOD_OFFS];

    if ((flags & mask) == mask) {
        bool left  = (flags & lmask) == lmask;
        bool right = (flags & rmask) == rmask;

        if (left)            add_flags(eventkey, hotkey_lrmod_flag[mod + LMOD_OFFS]);
        if (right)           add_flags(eventkey, hotkey_lrmod_flag[mod + RMOD_OFFS]);
        if (!left && !right) add_flags(eventkey, hotkey_lrmod_flag[mod]);
    }
}

void cgeventflags_to_hotkeyflags(CGEventFlags flags, struct hotkey *eventkey)
{
    cgevent_lrmod_flag_to_hotkey_lrmod_flag(flags, eventkey, LRMOD_ALT);
    cgevent_lrmod_flag_to_hotkey_lrmod_flag(flags, eventkey, LRMOD_CMD);
    cgevent_lrmod_flag_to_hotkey_lrmod_flag(flags, eventkey, LRMOD_CTRL);
    cgevent_lrmod_flag_to_hotkey_lrmod_flag(flags, eventkey, LRMOD_SHIFT);

    if ((flags & Event_Mask_Fn) == Event_Mask_Fn) {
        add_flags(eventkey, Hotkey_Flag_Fn);
    }
}
