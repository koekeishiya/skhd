#include "hotkey.h"

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

internal void
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
}

internal inline bool
passthrough(struct hotkey *hotkey)
{
    return !has_flags(hotkey, Hotkey_Flag_Passthrough);
}

internal inline struct hotkey *
find_hotkey(struct mode *mode, struct hotkey *hotkey)
{
    return table_find(&mode->hotkey_map, hotkey);
}

bool find_and_exec_hotkey(struct hotkey *k, struct table *t, struct mode **m)
{
    bool c = (*m)->capture;
    for (struct hotkey *h = find_hotkey(*m, k); h; c |= passthrough(h), h = 0) {
        char *cmd = h->command;
        if (has_flags(h, Hotkey_Flag_Activate)) {
            *m = table_find(t, cmd);
            cmd = (*m)->command;
        }
        if (cmd) fork_and_exec(cmd);
    }
    return c;
}

void free_mode_map(struct table *mode_map)
{
    struct hotkey **freed_pointers = NULL;

    int mode_count;
    void **modes = table_reset(mode_map, &mode_count);
    for (int mode_index = 0; mode_index < mode_count; ++mode_index) {
        struct mode *mode = (struct mode *) modes[mode_index];

        int hk_count;
        void **hotkeys = table_reset(&mode->hotkey_map, &hk_count);
        for (int hk_index = 0; hk_index < hk_count; ++hk_index) {
            struct hotkey *hotkey = (struct hotkey *) hotkeys[hk_index];

            for (int i = 0; i < buf_len(freed_pointers); ++i) {
                if (freed_pointers[i] == hotkey) {
                    goto next;
                }
            }

            buf_push(freed_pointers, hotkey);
            buf_free(hotkey->mode_list);
            free(hotkey->command);
            free(hotkey);
next:;
        }

        if (hk_count) free(hotkeys);
        if (mode->command) free(mode->command);
        if (mode->name) free(mode->name);
        free(mode);
    }

    if (mode_count) {
        free(modes);
        buf_free(freed_pointers);
    }
}

internal void
cgevent_lrmod_flag_to_hotkey_lrmod_flag(CGEventFlags eventflags, uint32_t *flags, int mod)
{
    enum osx_event_mask mask  = cgevent_lrmod_flag[mod];
    enum osx_event_mask lmask = cgevent_lrmod_flag[mod + LMOD_OFFS];
    enum osx_event_mask rmask = cgevent_lrmod_flag[mod + RMOD_OFFS];

    if ((eventflags & mask) == mask) {
        bool left  = (eventflags & lmask) == lmask;
        bool right = (eventflags & rmask) == rmask;

        if (left)            *flags |= hotkey_lrmod_flag[mod + LMOD_OFFS];
        if (right)           *flags |= hotkey_lrmod_flag[mod + RMOD_OFFS];
        if (!left && !right) *flags |= hotkey_lrmod_flag[mod];
    }
}

internal uint32_t
cgevent_flags_to_hotkey_flags(uint32_t eventflags)
{
    uint32_t flags = 0;

    cgevent_lrmod_flag_to_hotkey_lrmod_flag(eventflags, &flags, LRMOD_ALT);
    cgevent_lrmod_flag_to_hotkey_lrmod_flag(eventflags, &flags, LRMOD_CMD);
    cgevent_lrmod_flag_to_hotkey_lrmod_flag(eventflags, &flags, LRMOD_CTRL);
    cgevent_lrmod_flag_to_hotkey_lrmod_flag(eventflags, &flags, LRMOD_SHIFT);

    if ((eventflags & Event_Mask_Fn) == Event_Mask_Fn) {
        flags |= Hotkey_Flag_Fn;
    }

    return flags;
}

struct hotkey
create_eventkey(CGEventRef event)
{
    struct hotkey eventkey = {
        .key = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode),
        .flags = cgevent_flags_to_hotkey_flags(CGEventGetFlags(event))
    };
    return eventkey;
}
