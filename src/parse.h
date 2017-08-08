#ifndef SKHD_PARSE_H
#define SKHD_PARSE_H

#include "tokenize.h"
#include <stdbool.h>

struct parser
{
    struct token previous_token;
    struct token current_token;
    struct tokenizer tokenizer;
    bool error;
};

struct table;
void parse_config(struct parser *parser, struct table *hotkey_map);

struct token parser_peek(struct parser *parser);
struct token parser_previous(struct parser *parser);
bool parser_eof(struct parser *parser);
struct token parser_advance(struct parser *parser);
bool parser_check(struct parser *parser, enum token_type type);
bool parser_match(struct parser *parser, enum token_type type);
bool parser_init(struct parser *parser, char *file);
void parser_destroy(struct parser *parser);

#endif
