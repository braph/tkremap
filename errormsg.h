#ifndef _ERRORMSG_H
#define _ERRORMSG_H

#include <errno.h>

enum ErrorConstants {
  E_ERROR = 200,
  E_MISSING_ARG,
  E_UNKNOWN_OPT,
  E_UNKNOWN_CMD,
  E_UNKNOWN_MODE,
  E_INVALID_KEY,
  E_KEYCODE_NA,
  E_AMBIGIOUS_CMD,
  E_SYNTAX,
  E_SPARE_ARGS
};

char* error_get();
int   error_get_errno();
void  error_set_errno(int);
void  error_set(int, const char *cause);
void  error_setf(int, const char *fmt, ...);
void  error_add(const char *);
void  error_addf(const char *, ...);
void  error_free();

#endif

