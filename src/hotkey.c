#include "hotkey.h"
#include "locale.h"
#include "parse.h"

#include <string.h>
#include <pthread.h>

#define internal static
#define local_persist static

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

internal bool
compare_cmd(struct hotkey *a, struct hotkey *b)
{
    if(has_flags(a, Hotkey_Flag_Cmd)) {
        return (has_flags(b, Hotkey_Flag_LCmd) ||
                has_flags(b, Hotkey_Flag_RCmd) ||
                has_flags(b, Hotkey_Flag_Cmd));
    } else {
        return ((has_flags(a, Hotkey_Flag_LCmd) == has_flags(b, Hotkey_Flag_LCmd)) &&
                (has_flags(a, Hotkey_Flag_RCmd) == has_flags(b, Hotkey_Flag_RCmd)) &&
                (has_flags(a, Hotkey_Flag_Cmd) == has_flags(b, Hotkey_Flag_Cmd)));
    }
}

internal bool
compare_shift(struct hotkey *a, struct hotkey *b)
{
    if(has_flags(a, Hotkey_Flag_Shift)) {
        return (has_flags(b, Hotkey_Flag_LShift) ||
                has_flags(b, Hotkey_Flag_RShift) ||
                has_flags(b, Hotkey_Flag_Shift));
    } else {
        return ((has_flags(a, Hotkey_Flag_LShift) == has_flags(b, Hotkey_Flag_LShift)) &&
                (has_flags(a, Hotkey_Flag_RShift) == has_flags(b, Hotkey_Flag_RShift)) &&
                (has_flags(a, Hotkey_Flag_Shift) == has_flags(b, Hotkey_Flag_Shift)));
    }
}

internal bool
compare_alt(struct hotkey *a, struct hotkey *b)
{
    if(has_flags(a, Hotkey_Flag_Alt)) {
        return (has_flags(b, Hotkey_Flag_LAlt) ||
                has_flags(b, Hotkey_Flag_RAlt) ||
                has_flags(b, Hotkey_Flag_Alt));
    } else {
        return ((has_flags(a, Hotkey_Flag_LAlt) == has_flags(b, Hotkey_Flag_LAlt)) &&
                (has_flags(a, Hotkey_Flag_RAlt) == has_flags(b, Hotkey_Flag_RAlt)) &&
                (has_flags(a, Hotkey_Flag_Alt) == has_flags(b, Hotkey_Flag_Alt)));
    }
}

internal bool
compare_ctrl(struct hotkey *a, struct hotkey *b)
{
    if(has_flags(a, Hotkey_Flag_Control)) {
        return (has_flags(b, Hotkey_Flag_LControl) ||
                has_flags(b, Hotkey_Flag_RControl) ||
                has_flags(b, Hotkey_Flag_Control));
    } else {
        return ((has_flags(a, Hotkey_Flag_LControl) == has_flags(b, Hotkey_Flag_LControl)) &&
                (has_flags(a, Hotkey_Flag_RControl) == has_flags(b, Hotkey_Flag_RControl)) &&
                (has_flags(a, Hotkey_Flag_Control) == has_flags(b, Hotkey_Flag_Control)));
    }
}

internal bool
compare_fn(struct hotkey *a, struct hotkey *b)
{
    return has_flags(a, Hotkey_Flag_Fn) == has_flags(b, Hotkey_Flag_Fn);
}

bool same_hotkey(struct hotkey *a, struct hotkey *b)
{
    return compare_cmd(a, b) &&
           compare_shift(a, b) &&
           compare_alt(a, b) &&
           compare_ctrl(a, b) &&
           compare_fn(a, b) &&
           a->key == b->key;
}

unsigned long hash_hotkey(struct hotkey *a)
{
    return a->key;
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

void cgeventflags_to_hotkeyflags(CGEventFlags flags, struct hotkey *eventkey)
{
    if((flags & Event_Mask_Cmd) == Event_Mask_Cmd) {
        bool left = (flags & Event_Mask_LCmd) == Event_Mask_LCmd;
        bool right = (flags & Event_Mask_RCmd) == Event_Mask_RCmd;

        if(left)            add_flags(eventkey, Hotkey_Flag_LCmd);
        if(right)           add_flags(eventkey, Hotkey_Flag_RCmd);
        if(!left && !right) add_flags(eventkey, Hotkey_Flag_Cmd);
    }

    if((flags & Event_Mask_Shift) == Event_Mask_Shift) {
        bool left = (flags & Event_Mask_LShift) == Event_Mask_LShift;
        bool right = (flags & Event_Mask_RShift) == Event_Mask_RShift;

        if(left)            add_flags(eventkey, Hotkey_Flag_LShift);
        if(right)           add_flags(eventkey, Hotkey_Flag_RShift);
        if(!left && !right) add_flags(eventkey, Hotkey_Flag_Shift);
    }

    if((flags & Event_Mask_Alt) == Event_Mask_Alt) {
        bool left = (flags & Event_Mask_LAlt) == Event_Mask_LAlt;
        bool right = (flags & Event_Mask_RAlt) == Event_Mask_RAlt;

        if(left)            add_flags(eventkey, Hotkey_Flag_LAlt);
        if(right)           add_flags(eventkey, Hotkey_Flag_RAlt);
        if(!left && !right) add_flags(eventkey, Hotkey_Flag_Alt);
    }

    if((flags & Event_Mask_Control) == Event_Mask_Control) {
        bool left = (flags & Event_Mask_LControl) == Event_Mask_LControl;
        bool right = (flags & Event_Mask_RControl) == Event_Mask_RControl;

        if(left)            add_flags(eventkey, Hotkey_Flag_LControl);
        if(right)           add_flags(eventkey, Hotkey_Flag_RControl);
        if(!left && !right) add_flags(eventkey, Hotkey_Flag_Control);
    }

    if((flags & Event_Mask_Fn) == Event_Mask_Fn) {
        add_flags(eventkey, Hotkey_Flag_Fn);
    }
}
