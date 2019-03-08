#define _GNU_SOURCE
#include "errormsg.h"
#include "common.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

const char* E_MISSING_ARG   = "missing argument";
const char* E_UNKNOWN_OPT   = "unknown option";
const char* E_UNKNOWN_CMD   = "unknown command";
const char* E_INVALID_KEY   = "invalid key";
const char* E_KEYCODE_NA    = "could not get key code";
const char* E_AMBIGIOUS_CMD = "ambigious command";

static char* error_msg;
static int   error_sz;

char *error_get() {
  return error_msg;
}

void error_write(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int need_resize = vsnprintf(error_msg, error_sz, fmt, ap);
  va_end(ap);

  if (need_resize >= error_sz) {
    free(error_msg);
    va_start(ap, fmt);
    error_sz = vasprintf(&error_msg, fmt, ap) + 1;
    va_end(ap);
  }
}

#define MAX 2048
#define SEPERATOR ": "
#define SEPERATOR_LEN (sizeof(SEPERATOR) - 1)

void error_add(const char *fmt, ...) {
  // Build "added error: "
  char temp[MAX + SEPERATOR_LEN + 1];
  va_list ap;
  va_start(ap, fmt);
  int templen = vsnprintf(temp, MAX, fmt, ap) + SEPERATOR_LEN;
  va_end(ap);
  strcat(temp, SEPERATOR);

  if (! error_msg)
    error_msg = strdup("NO PREVIOUS ERROR");

  int oldlen  = strlen(error_msg);
  int newsize = templen + oldlen + 1;

  if (newsize>= error_sz) {
    error_msg = realloc(error_msg, newsize);
    error_sz  = newsize;
  }

  memmove(error_msg + templen, error_msg, oldlen + 1);
  memcpy(error_msg, temp, templen);
}

void error_free() {
  free(error_msg);
  error_msg = 0;
}
