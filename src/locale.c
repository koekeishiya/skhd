#include "locale.h"
#include "hashtable.h"
#include <Carbon/Carbon.h>
#include <IOKit/hidsystem/ev_keymap.h>

#define internal static
#define local_persist static

internal struct table keymap_table;

internal char *
copy_cf_string_to_c(CFStringRef string)
{
    CFStringEncoding encoding = kCFStringEncodingUTF8;
    CFIndex length = CFStringGetLength(string);
    CFIndex bytes = CFStringGetMaximumSizeForEncoding(length, encoding);
    char *result = malloc(bytes + 1);

    // NOTE(koekeishiya): Boolean: typedef -> unsigned char; false = 0, true != 0
    Boolean success = CFStringGetCString(string, result, bytes + 1, encoding);
    if (!success) {
        free(result);
        result = NULL;
    }

    return result;
}

internal int
hash_keymap(const char *a)
{
    unsigned long hash = 0, high;
    while(*a) {
        hash = (hash << 4) + *a++;
        high = hash & 0xF0000000;
        if(high) {
            hash ^= (high >> 24);
        }
        hash &= ~high;
    }
    return hash;
}

internal bool
same_keymap(const char *a, const char *b)
{
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

internal CFStringRef
cfstring_from_keycode(CGKeyCode keycode)
{
    TISInputSourceRef keyboard = TISCopyCurrentASCIICapableKeyboardLayoutInputSource();
    CFDataRef uchr = (CFDataRef) TISGetInputSourceProperty(keyboard, kTISPropertyUnicodeKeyLayoutData);
    CFRelease(keyboard);

    UCKeyboardLayout *keyboard_layout = (UCKeyboardLayout *) CFDataGetBytePtr(uchr);
    if (keyboard_layout) {
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

        if (string_length == 0 && dead_key_state) {
            status = UCKeyTranslate(keyboard_layout, kVK_Space,
                                    kUCKeyActionDown, 0,
                                    LMGetKbdType(), 0,
                                    &dead_key_state,
                                    max_string_length,
                                    &string_length,
                                    unicode_string);
        }

        if (string_length > 0 && status == noErr) {
            return CFStringCreateWithCharacters(NULL, unicode_string, string_length);
        }
    }

    return NULL;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"
internal void
initialize_keycode_map()
{
    table_init(&keymap_table,
               131,
               (table_hash_func) hash_keymap,
               (table_compare_func) same_keymap);

    for (unsigned index = 0; index < 128; ++index) {
        CFStringRef key_string = cfstring_from_keycode(index);
        if (!key_string) continue;

        char *c_key_string = copy_cf_string_to_c(key_string);
        CFRelease(key_string);
        if (!c_key_string) continue;

        table_add(&keymap_table, c_key_string, (void *)index);
    }
}
#pragma clang diagnostic pop

uint32_t keycode_from_char(char key)
{
    uint32_t keycode = 0;
    char lookup_key[2];

    if (!keymap_table.count) {
        initialize_keycode_map();
    }

    snprintf(lookup_key, 2, "%c", key);
    keycode = (uint32_t) table_find(&keymap_table, &lookup_key);

    return keycode;
}
