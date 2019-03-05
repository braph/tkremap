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

static char* error_msg = NULL;
#define      ERROR_MSG_MAX 2048

char *get_error() {
   return error_msg;
}

void write_error(const char *fmt, ...) {
   error_msg = realloc(error_msg, ERROR_MSG_MAX);

   va_list ap;
   va_start(ap, fmt);
   vsnprintf(error_msg, ERROR_MSG_MAX, fmt, ap);
   va_end(ap);
}

void prepend_error(const char *fmt, ...) {
   char *old_error = error_msg;
   char temp[ERROR_MSG_MAX];

   va_list ap;
   va_start(ap, fmt);
   int l = vsnprintf(temp, ERROR_MSG_MAX, fmt, ap);
   va_end(ap);

   error_msg = malloc(sizeof(": ") + strlen(old_error) + l);
   sprintf(error_msg, "%s: %s", temp, old_error);
   free(old_error);
}

#if FREE_MEMORY
void free_error() {
   free(error_msg);
}
#endif
