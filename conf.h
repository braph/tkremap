#ifndef _CONF_H
#define _CONF_H

#include <stdio.h>
#include "tkremap.h"

int read_conf_file(const char *);
int read_conf_string(const char *);
int read_conf_stream(FILE *);

#endif
