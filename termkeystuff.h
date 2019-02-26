#ifndef _TERMKEYSTUFF_H
#define _TERMKEYSTUFF_H

#include <termkey.h>

extern         TermKey *tk;
int            load_terminfo();
int            parse_key(const char *def, TermKeyKey *key);
TermKeyKey*    parse_key_new(const char *def);
const char*    format_key(TermKeyKey *key);
const char*    get_key_code(TermKeyKey *key);

#if FREE_MEMORY
void           unload_terminfo();
#endif

#endif
