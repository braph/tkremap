#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "tkremap.h"
#include <stdio.h>

extern command_t* commands[];
#define COMMANDS_SIZE (18 + !!DEBUG) // DEBUG -> command_debug

const command_t*  get_command(const char*);
command_call_t*   command_parse(void*, command_call_t*);

#endif
