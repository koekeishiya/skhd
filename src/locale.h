#ifndef SKHD_LOCALE_H
#define SKHD_LOCALE_H

#include <stdbool.h>
#include <stdint.h>

uint32_t keycode_from_char(char key);
uint32_t keycode_from_literal(char *key, unsigned length);
bool same_string(char *text, unsigned length, const char *match);

#endif
