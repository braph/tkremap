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
    va_start(ap, fmt);
    error_sz = vasprintf(&error_msg, fmt, ap) + 1;
    va_end(ap);
  }
}

#define MAX 2048
void error_add(const char *fmt, ...) {
  char *olderror = error_msg;
  char temp[MAX];
  va_list ap;
  va_start(ap, fmt);
  snprintf(temp, MAX, fmt, ap);
  va_end(ap);

  error_sz = asprintf(&error_msg, "%s: %s", temp, olderror) + 1;
  free(olderror);
}

void error_free() {
  free(error_msg);
  error_msg = 0;
}
