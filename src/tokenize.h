#ifndef SKHD_TOKENIZE_H
#define SKHD_TOKENIZE_H

#define array_count(a) (sizeof((a)) / sizeof(*(a)))
static const char *modifier_flags_str[] =
{
    "alt",   "lalt",    "ralt",
    "shift", "lshift",  "rshift",
    "cmd",   "lcmd",    "rcmd",
    "ctrl",  "lctrl",   "rctrl",
};

static const char *literal_keycode_str[] =
{
    "return",     "tab",        "space",
    "backspace",  "delete",     "escape",
    "home",       "end",        "pageup",
    "pagedown",   "help",       "left",
    "right",      "up",         "down",
    "f1",         "f2",         "f3",
    "f4",         "f5",         "f6",
    "f7",         "f8",         "f9",
    "f10",        "f11",        "f12",
    "f13",        "f14",        "f15",
    "f16",        "f17",        "f18",
    "f19",        "f20",
};

enum token_type
{
    Token_Command,
    Token_Modifier,
    Token_Literal,
    Token_Key_Hex,
    Token_Key,

    Token_Plus,
    Token_Dash,
    Token_Arrow,

    Token_Unknown,
    Token_EndOfStream,
};

struct token
{
    enum token_type type;
    char *text;
    unsigned length;

    unsigned line;
    unsigned cursor;
};

struct tokenizer
{
    char *buffer;
    char *at;
    unsigned line;
    unsigned cursor;
};

void tokenizer_init(struct tokenizer *tokenizer, char *buffer);
struct token get_token(struct tokenizer *tokenizer);
struct token peek_token(struct tokenizer tokenizer);

#endif
