#define _GNU_SOURCE
#include "errormsg.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

const char * const E_MISSING_ARG   = "missing argument";
const char * const E_UNKNOWN_OPT   = "unknown option";
const char * const E_UNKNOWN_CMD   = "unknown command";
const char * const E_INVALID_KEY   = "invalid key";
const char * const E_KEYCODE_NA    = "could not get key code";
const char * const E_AMBIGIOUS_CMD = "ambigious command";

static char* _error_msg;

char *error_get() {
   return _error_msg;
}

void error_write(const char *fmt, ...) {
   free(_error_msg);
   va_list ap;
   va_start(ap, fmt);
   vasprintf(&_error_msg, fmt, ap);
   va_end(ap);
}

void error_add(const char *fmt, ...) {
   char *old_error = _error_msg;
   char *temp;
   va_list ap;
   va_start(ap, fmt);
   vasprintf(&temp, fmt, ap);
   va_end(ap);

   asprintf(&_error_msg, "%s: %s", temp, old_error);
   free(temp);
   free(old_error);
}

void error_free() {
   free(_error_msg);
   _error_msg = 0;
}
