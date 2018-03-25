#ifndef SKHD_LOCALE_H
#define SKHD_LOCALE_H

#include <stdint.h>

bool initialize_keycode_map();
uint32_t keycode_from_char(char key);

#endif
