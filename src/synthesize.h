#ifndef SKHD_SYNTHESIZE_H
#define SKHD_SYNTHESIZE_H

void synthesize_key(char *key_string);
void synthesize_text(char *text);

void synthesize_forward_hotkey(struct hotkey *hotkey);
#define SHKD_CGEVENT_USER_DATA 0x12345

#endif
