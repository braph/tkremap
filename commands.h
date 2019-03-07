#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "tkremap.h"
#include <stdio.h>

extern command_t* commands[];
extern int        commands_size;

command_t*       get_command(const char *);
command_call_t*  command_parse(int, char **, command_call_t*);
commands_t*      commands_parse(int, char **);

#endif
