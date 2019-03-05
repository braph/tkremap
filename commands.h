#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "tkremap.h"
#include <stdio.h>

extern command_t* commands[];
extern int        commands_size;

void*      command_parse(command_t *, int, char **);
command_t* get_command(const char *);

#endif
