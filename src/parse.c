#include "parse.h"
#include "tokenize.h"
#include "locale.h"
#include "hotkey.h"
#include "hashtable.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define internal static

internal char *
read_file(const char *file)
{
    unsigned length;
    char *buffer = NULL;
    FILE *handle = fopen(file, "r");

    if(handle)
    {
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
    printf("\tkey: '%.*s' (%d)\n", key.length, key.text, keycode);
    return keycode;
}

internal uint32_t
parse_key(struct parser *parser)
{
    uint32_t keycode;
    struct token key = parser_previous(parser);
    if(key.length == 1) {
        keycode = keycode_from_char(*key.text);
    } else {
        keycode = keycode_from_literal(key.text, key.length);
    }
    printf("\tkey: '%.*s' (%d)\n", key.length, key.text, keycode);
    return keycode;
}

internal const char *modifier_flags_map[] =
{
    "alt",   "lalt",    "ralt",
    "shift", "lshift",  "rshift",
    "cmd",   "lcmd",    "rcmd",
    "ctrl",  "lctrl",   "rctrl",
};
internal enum hotkey_flag modifier_flags_value[] =
{
    Hotkey_Flag_Alt,        Hotkey_Flag_LAlt,       Hotkey_Flag_RAlt,
    Hotkey_Flag_Shift,      Hotkey_Flag_LShift,     Hotkey_Flag_RShift,
    Hotkey_Flag_Cmd,        Hotkey_Flag_LCmd,       Hotkey_Flag_RCmd,
    Hotkey_Flag_Control,    Hotkey_Flag_LControl,   Hotkey_Flag_RControl,
};

internal uint32_t
parse_modifier(struct parser *parser)
{
    uint32_t flags = 0;
    struct token modifier = parser_previous(parser);

    for(int i = 0; i < array_count(modifier_flags_map); ++i) {
        if(same_string(modifier.text, modifier.length, modifier_flags_map[i])) {
            flags |= modifier_flags_value[i];
            printf("\tmod: '%s'\n", modifier_flags_map[i]);
            break;
        }
    }

    if(parser_match(parser, Token_Plus)) {
        if(parser_match(parser, Token_Modifier)) {
            flags |= parse_modifier(parser);
        } else {
            fprintf(stderr, "(#%d:%d) expected token 'Token_Modifier', but got '%.*s'\n",
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

    if(parser_match(parser, Token_Modifier)) {
        hotkey->flags = parse_modifier(parser);
        if(parser->error) {
            return NULL;
        }
        found_modifier = 1;
    } else {
        hotkey->flags = found_modifier = 0;
    }

    if(found_modifier) {
        if(!parser_match(parser, Token_Dash)) {
            fprintf(stderr, "(#%d:%d) expected token '-', but got '%.*s'\n",
                    parser->current_token.line, parser->current_token.cursor,
                    parser->current_token.length, parser->current_token.text);
            parser->error = true;
            return NULL;
        }
    }

    if(parser_match(parser, Token_Key)) {
        hotkey->key = parse_key(parser);
    } else if(parser_match(parser, Token_Key_Hex)) {
        hotkey->key = parse_key_hex(parser);
    } else {
        fprintf(stderr, "(#%d:%d) expected token 'Token_Key', but got '%.*s'\n",
                parser->current_token.line, parser->current_token.cursor,
                parser->current_token.length, parser->current_token.text);
        parser->error = true;
        return NULL;
    }

    if(parser_match(parser, Token_Arrow)) {
        hotkey->flags |= Hotkey_Flag_Passthrough;
    }

    if(parser_match(parser, Token_Command)) {
        hotkey->command = parse_command(parser);
    } else {
        fprintf(stderr, "(#%d:%d) expected token 'Token_Command', but got '%.*s'\n",
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
    while(!parser_eof(parser)) {
        if((parser_check(parser, Token_Modifier)) ||
           (parser_check(parser, Token_Key_Hex)) ||
           (parser_check(parser, Token_Key))) {
            hotkey = parse_hotkey(parser);
            table_add(hotkey_map, hotkey, hotkey);
            if(parser->error) {
                free_hotkeys(hotkey_map);
                return;
            }
        } else {
            fprintf(stderr, "(#%d:%d) expected token 'Token_Modifier', 'Token_Key_Hex' or 'Token_Key', but got '%.*s'\n",
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
    if(!parser_eof(parser)) {
        parser->previous_token = parser->current_token;
        parser->current_token = get_token(&parser->tokenizer);
    }
    return parser_previous(parser);
}

bool parser_check(struct parser *parser, enum token_type type)
{
    if(parser_eof(parser)) return 0;
    struct token token = parser_peek(parser);
    return token.type == type;
}

bool parser_match(struct parser *parser, enum token_type type)
{
    if(parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

bool parser_init(struct parser *parser, char *file)
{
    memset(parser, 0, sizeof(struct parser));
    char *buffer = read_file(file);
    if(buffer) {
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
