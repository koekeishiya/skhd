#include "locale.h"

#include <Carbon/Carbon.h>
#include <IOKit/hidsystem/ev_keymap.h>

#define internal static
#define local_persist static

bool same_string(char *text, unsigned length, const char *match)
{
    const char *at = match;
    unsigned index = 0;
    while(*at++ == text[index++] && index < length);
    return (*at == '\0' && index == length) ? true : false;
}

internal CFStringRef
cfstring_from_keycode(CGKeyCode keycode)
{
    TISInputSourceRef keyboard = TISCopyCurrentASCIICapableKeyboardLayoutInputSource();
    CFDataRef uchr = (CFDataRef) TISGetInputSourceProperty(keyboard, kTISPropertyUnicodeKeyLayoutData);
    CFRelease(keyboard);

    UCKeyboardLayout *keyboard_layout = (UCKeyboardLayout *) CFDataGetBytePtr(uchr);
    if(keyboard_layout) {
        UInt32 dead_key_state = 0;
        UniCharCount max_string_length = 255;
        UniCharCount string_length = 0;
        UniChar unicode_string[max_string_length];

        OSStatus status = UCKeyTranslate(keyboard_layout, keycode,
                                         kUCKeyActionDown, 0,
                                         LMGetKbdType(), 0,
                                         &dead_key_state,
                                         max_string_length,
                                         &string_length,
                                         unicode_string);

        if(string_length == 0 && dead_key_state) {
            status = UCKeyTranslate(keyboard_layout, kVK_Space,
                                    kUCKeyActionDown, 0,
                                    LMGetKbdType(), 0,
                                    &dead_key_state,
                                    max_string_length,
                                    &string_length,
                                    unicode_string);
        }

        if(string_length > 0 && status == noErr) {
            return CFStringCreateWithCharacters(NULL, unicode_string, string_length);
        }
    }

    return NULL;
}

uint32_t keycode_from_char(char key)
{
    uint32_t keycode = 0;
    local_persist CFMutableDictionaryRef keycode_map = NULL;
    if(!keycode_map) {
        keycode_map = CFDictionaryCreateMutable(kCFAllocatorDefault, 128, &kCFCopyStringDictionaryKeyCallBacks, NULL);
        for(unsigned index = 0; index < 128; ++index) {
            CFStringRef key_string = cfstring_from_keycode(index);
            if(key_string) {
                CFDictionaryAddValue(keycode_map, key_string, (const void *)index);
                CFRelease(key_string);
            }
        }
    }

    UniChar uni_char = key;
    CFStringRef char_str = CFStringCreateWithCharacters(kCFAllocatorDefault, &uni_char, 1);
    CFDictionaryGetValueIfPresent(keycode_map, char_str, (const void **)&keycode);
    CFRelease(char_str);

    return keycode;
}

uint32_t keycode_from_literal(char *key, unsigned length)
{
    if(same_string(key, length, "return")) {
        return kVK_Return;
    } else if(same_string(key, length, "tab")) {
        return kVK_Tab;
    } else if(same_string(key, length, "space")) {
        return kVK_Space;
    } else if(same_string(key, length, "backspace")) {
        return kVK_Delete;
    } else if(same_string(key, length, "delete")) {
        return kVK_ForwardDelete;
    } else if(same_string(key, length, "escape")) {
        return kVK_Escape;
    } else if(same_string(key, length, "home")) {
        return kVK_Home;
    } else if(same_string(key, length, "end")) {
        return kVK_End;
    } else if(same_string(key, length, "pageup")) {
        return kVK_PageUp;
    } else if(same_string(key, length, "pagedown")) {
        return kVK_PageDown;
    } else if(same_string(key, length, "help")) {
        return kVK_Help;
    } else if(same_string(key, length, "left")) {
        return kVK_LeftArrow;
    } else if(same_string(key, length, "right")) {
        return kVK_RightArrow;
    } else if(same_string(key, length, "up")) {
        return kVK_UpArrow;
    } else if(same_string(key, length, "down")) {
        return kVK_DownArrow;
    } else if(same_string(key, length, "f1")) {
        return kVK_F1;
    } else if(same_string(key, length, "f2")) {
        return kVK_F2;
    } else if(same_string(key, length, "f3")) {
        return kVK_F3;
    } else if(same_string(key, length, "f4")) {
        return kVK_F4;
    } else if(same_string(key, length, "f5")) {
        return kVK_F5;
    } else if(same_string(key, length, "f6")) {
        return kVK_F6;
    } else if(same_string(key, length, "f7")) {
        return kVK_F7;
    } else if(same_string(key, length, "f8")) {
        return kVK_F8;
    } else if(same_string(key, length, "f9")) {
        return kVK_F9;
    } else if(same_string(key, length, "f10")) {
        return kVK_F10;
    } else if(same_string(key, length, "f11")) {
        return kVK_F11;
    } else if(same_string(key, length, "f12")) {
        return kVK_F12;
    } else {
        return 0;
    }
}
