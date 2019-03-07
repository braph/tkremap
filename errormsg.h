#ifndef _ERRORMSG_H
#define _ERRORMSG_H

extern const char *E_MISSING_ARG;
extern const char *E_UNKNOWN_OPT;
extern const char *E_UNKNOWN_CMD;
extern const char *E_INVALID_KEY;
extern const char *E_KEYCODE_NA;
extern const char *E_AMBIGIOUS_CMD;

char* error_get();
void  error_write(const char *fmt, ...);
void  error_add(const char *fmt, ...);
void  error_free();

#endif

