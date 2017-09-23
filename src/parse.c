#include "parse.h"
#include "tokenize.h"
#include "locale.h"
#include "hotkey.h"
#include "hashtable.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define internal static

internal char *
read_file(const char *file)
{
    unsigned length;
    char *buffer = NULL;
    FILE *handle = fopen(file, "r");

    if (handle) {
        fseek(handle, 0, SEEK_END);
        length = ftell(handle);
        fseek(handle, 0, SEEK_SET);
        buffer = malloc(length + 1);
        fread(buffer, length, 1, handle);
        buffer[length] = '\0';
        fclose(handle);
    }

    return buffer;
}

internal char *
copy_string_count(char *s, int length)
{
    char *result = malloc(length + 1);
    memcpy(result, s, length);
    result[length] = '\0';
    return result;
}

internal uint32_t
keycode_from_hex(char *hex)
{
    uint32_t result;
    sscanf(hex, "%x", &result);
    return result;
}

internal char *
parse_command(struct parser *parser)
{
    struct token command = parser_previous(parser);
    char *result = copy_string_count(command.text, command.length);
    printf("\tcmd: '%s'\n", result);
    return result;
}

internal uint32_t
parse_key_hex(struct parser *parser)
{
    struct token key = parser_previous(parser);
    char *hex = copy_string_count(key.text, key.length);
    uint32_t keycode = keycode_from_hex(hex);
    free(hex);
    printf("\tkey: '%.*s' (0x%02x)\n", key.length, key.text, keycode);
    return keycode;
}

internal uint32_t
parse_key(struct parser *parser)
{
    uint32_t keycode;
    struct token key = parser_previous(parser);
    keycode = keycode_from_char(*key.text);
    printf("\tkey: '%c' (0x%02x)\n", *key.text, keycode);
    return keycode;
}

internal uint32_t literal_keycode_value[] =
{
    kVK_Return,     kVK_Tab,           kVK_Space,
    kVK_Delete,     kVK_ForwardDelete, kVK_Escape,
    kVK_Home,       kVK_End,           kVK_PageUp,
    kVK_PageDown,   kVK_Help,          kVK_LeftArrow,
    kVK_RightArrow, kVK_UpArrow,       kVK_DownArrow,
    kVK_F1,         kVK_F2,            kVK_F3,
    kVK_F4,         kVK_F5,            kVK_F6,
    kVK_F7,         kVK_F8,            kVK_F9,
    kVK_F10,        kVK_F11,           kVK_F12,
    kVK_F13,        kVK_F14,           kVK_F15,
    kVK_F16,        kVK_F17,           kVK_F18,
    kVK_F19,        kVK_F20,
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsometimes-uninitialized"
// NOTE(koekeishiya): shut up compiler !!!
// if we get to this point, we already KNOW that the input is valid..
internal uint32_t
parse_key_literal(struct parser *parser)
{
    uint32_t keycode;
    struct token key = parser_previous(parser);

    for (int i = 0; i < array_count(literal_keycode_str); ++i) {
        if (token_equals(key, literal_keycode_str[i])) {
            keycode = literal_keycode_value[i];
            printf("\tkey: '%.*s' (0x%02x)\n", key.length, key.text, keycode);
            break;
        }
    }

    return keycode;
}
#pragma clang diagnostic pop

internal enum hotkey_flag modifier_flags_value[] =
{
    Hotkey_Flag_Alt,        Hotkey_Flag_LAlt,       Hotkey_Flag_RAlt,
    Hotkey_Flag_Shift,      Hotkey_Flag_LShift,     Hotkey_Flag_RShift,
    Hotkey_Flag_Cmd,        Hotkey_Flag_LCmd,       Hotkey_Flag_RCmd,
    Hotkey_Flag_Control,    Hotkey_Flag_LControl,   Hotkey_Flag_RControl,
    Hotkey_Flag_Fn,         Hotkey_Flag_Hyper,
};

internal uint32_t
parse_modifier(struct parser *parser)
{
    uint32_t flags = 0;
    struct token modifier = parser_previous(parser);

    for (int i = 0; i < array_count(modifier_flags_str); ++i) {
        if (token_equals(modifier, modifier_flags_str[i])) {
            flags |= modifier_flags_value[i];
            printf("\tmod: '%s'\n", modifier_flags_str[i]);
            break;
        }
    }

    if (parser_match(parser, Token_Plus)) {
        if (parser_match(parser, Token_Modifier)) {
            flags |= parse_modifier(parser);
        } else {
            fprintf(stderr, "(#%d:%d) expected modifier, but got '%.*s'\n",
                    parser->current_token.line, parser->current_token.cursor,
                    parser->current_token.length, parser->current_token.text);
            parser->error = true;
        }
    }

    return flags;
}

internal struct hotkey *
parse_hotkey(struct parser *parser)
{
    struct hotkey *hotkey = malloc(sizeof(struct hotkey));
    int found_modifier;

    printf("(#%d) hotkey :: {\n", parser->current_token.line);

    if (parser_match(parser, Token_Modifier)) {
        hotkey->flags = parse_modifier(parser);
        if (parser->error) {
            return NULL;
        }
        found_modifier = 1;
    } else {
        hotkey->flags = found_modifier = 0;
    }

    if (found_modifier) {
        if (!parser_match(parser, Token_Dash)) {
            fprintf(stderr, "(#%d:%d) expected '-', but got '%.*s'\n",
                    parser->current_token.line, parser->current_token.cursor,
                    parser->current_token.length, parser->current_token.text);
            parser->error = true;
            return NULL;
        }
    }

    if (parser_match(parser, Token_Key)) {
        hotkey->key = parse_key(parser);
    } else if (parser_match(parser, Token_Key_Hex)) {
        hotkey->key = parse_key_hex(parser);
    } else if (parser_match(parser, Token_Literal)) {
        hotkey->key = parse_key_literal(parser);
    } else {
        fprintf(stderr, "(#%d:%d) expected key-literal, but got '%.*s'\n",
                parser->current_token.line, parser->current_token.cursor,
                parser->current_token.length, parser->current_token.text);
        parser->error = true;
        return NULL;
    }

    if (parser_match(parser, Token_Arrow)) {
        hotkey->flags |= Hotkey_Flag_Passthrough;
    }

    if (parser_match(parser, Token_Command)) {
        hotkey->command = parse_command(parser);
    } else {
        fprintf(stderr, "(#%d:%d) expected ':' followed by command, but got '%.*s'\n",
                parser->current_token.line, parser->current_token.cursor,
                parser->current_token.length, parser->current_token.text);
        parser->error = true;
        return NULL;
    }

    printf("}\n");

    return hotkey;
}

void parse_config(struct parser *parser, struct table *hotkey_map)
{
    struct hotkey *hotkey;
    while (!parser_eof(parser)) {
        if ((parser_check(parser, Token_Modifier)) ||
            (parser_check(parser, Token_Literal)) ||
            (parser_check(parser, Token_Key_Hex)) ||
            (parser_check(parser, Token_Key))) {
            hotkey = parse_hotkey(parser);
            if (parser->error) {
                free_hotkeys(hotkey_map);
                return;
            }
            table_add(hotkey_map, hotkey, hotkey);
        } else {
            fprintf(stderr, "(#%d:%d) expected modifier or key-literal, but got '%.*s'\n",
                    parser->current_token.line, parser->current_token.cursor,
                    parser->current_token.length, parser->current_token.text);
            parser->error = true;
            return;
        }
    }
}

struct token
parser_peek(struct parser *parser)
{
    return parser->current_token;
}

struct token
parser_previous(struct parser *parser)
{
    return parser->previous_token;
}

bool parser_eof(struct parser *parser)
{
    struct token token = parser_peek(parser);
    return token.type == Token_EndOfStream;
}

struct token
parser_advance(struct parser *parser)
{
    if (!parser_eof(parser)) {
        parser->previous_token = parser->current_token;
        parser->current_token = get_token(&parser->tokenizer);
    }
    return parser_previous(parser);
}

bool parser_check(struct parser *parser, enum token_type type)
{
    if (parser_eof(parser)) return false;
    struct token token = parser_peek(parser);
    return token.type == type;
}

bool parser_match(struct parser *parser, enum token_type type)
{
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

bool parser_init(struct parser *parser, char *file)
{
    memset(parser, 0, sizeof(struct parser));
    char *buffer = read_file(file);
    if (buffer) {
        tokenizer_init(&parser->tokenizer, buffer);
        parser_advance(parser);
        return true;
    }
    return false;
}

void parser_destroy(struct parser *parser)
{
    free(parser->tokenizer.buffer);
}
