#include <Carbon/Carbon.h>

#include "synthesize.h"
#include "locale.h"
#include "parse.h"
#include "hotkey.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"

static inline void
create_and_post_keyevent(uint16_t key, bool pressed)
{
    CGPostKeyboardEvent((CGCharCode)0, (CGKeyCode)key, pressed);
}

static inline void
synthesize_modifiers(struct hotkey *hotkey, bool pressed)
{
    if (has_flags(hotkey, Hotkey_Flag_Alt)) {
        create_and_post_keyevent(Modifier_Keycode_Alt, pressed);
    }

    if (has_flags(hotkey, Hotkey_Flag_Shift)) {
        create_and_post_keyevent(Modifier_Keycode_Shift, pressed);
    }

    if (has_flags(hotkey, Hotkey_Flag_Cmd)) {
        create_and_post_keyevent(Modifier_Keycode_Cmd, pressed);
    }

    if (has_flags(hotkey, Hotkey_Flag_Control)) {
        create_and_post_keyevent(Modifier_Keycode_Ctrl, pressed);
    }

    if (has_flags(hotkey, Hotkey_Flag_Fn)) {
        create_and_post_keyevent(Modifier_Keycode_Fn, pressed);
    }
}

void synthesize_key(char *key_string)
{
    if (!initialize_keycode_map()) return;

    struct parser parser;
    parser_init_text(&parser, key_string);

    close(1);
    close(2);

    struct hotkey *hotkey = parse_keypress(&parser);
    if (!hotkey) return;

    CGSetLocalEventsSuppressionInterval(0.0f);
    CGEnableEventStateCombining(false);

    synthesize_modifiers(hotkey, true);
    create_and_post_keyevent(hotkey->key, true);

    create_and_post_keyevent(hotkey->key, false);
    synthesize_modifiers(hotkey, false);
}

void synthesize_text(char *text)
{
    CFStringRef text_ref = CFStringCreateWithCString(NULL, text, kCFStringEncodingUTF8);
    CFIndex text_length = CFStringGetLength(text_ref);

    CGEventRef de = CGEventCreateKeyboardEvent(NULL, 0, true);
    CGEventRef ue = CGEventCreateKeyboardEvent(NULL, 0, false);

    CGEventSetFlags(de, 0);
    CGEventSetFlags(ue, 0);

    UniChar c;
    for (CFIndex i = 0; i < text_length; ++i)
    {
        c = CFStringGetCharacterAtIndex(text_ref, i);
        CGEventKeyboardSetUnicodeString(de, 1, &c);
        CGEventPost(kCGAnnotatedSessionEventTap, de);
        usleep(1000);
        CGEventKeyboardSetUnicodeString(ue, 1, &c);
        CGEventPost(kCGAnnotatedSessionEventTap, ue);
    }

    CFRelease(ue);
    CFRelease(de);
    CFRelease(text_ref);
}

#pragma clang diagnostic pop
