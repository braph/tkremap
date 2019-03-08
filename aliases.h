#ifndef _ALIASES_H
#define _ALIASES_H

#include "tkremap.h"

typedef struct alias_t {
  char  *name;
  char  *desc;
  char **definition;
} alias_t;

extern alias_t  **aliases;
extern size_t   aliases_size;

alias_t* alias_get(const char *);
char**   alias_resolve(const char *, int *, char *[]);
void     alias_register(const char*, const char*, int, const char**);
void     alias_init();

#endif
