#include "errormsg.h"
#include "common.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>

#define ERROR_MESSAGE_MAX_LEN         1024
#define ERROR_ADDITIONAL_MESSAGES_MAX 8

static struct errors_t {
  uint16_t  error_no;
  char*     cause;

  char*     additional_msgs[ERROR_ADDITIONAL_MESSAGES_MAX];
  uint16_t  additional_msgs_sz;

  char*     error_msg; // final error message
} this;

/*
 * error_set_errno(int)       -> only set error number
 * error_set(int, cause)      -> set error number and cause
 * error_setf(int, fmt, ...)  -> set error number and cause (printf)
 * error_add(caller)          -> add error message from caller
 * error_addf(fmt, ...)       -> add error message from caller, printf
 */

// Return string representation of error code
const char* error_strerror(int e) {
  switch (e) {
    case (int) E_UNKNOWN_OPT:   return "Unknown option";
    case (int) E_UNKNOWN_CMD:   return "Unknown command";
    case (int) E_UNKNOWN_MODE:  return "Unknown mode";
    case (int) E_MISSING_ARG:   return "Missing argument";
    case (int) E_INVALID_KEY:   return "Invalid key";
    case (int) E_KEYCODE_NA:    return "Could not get key code";
    case (int) E_SPARE_ARGS:    return "Too much arguments provided for command";
    case (int) E_AMBIGIOUS_CMD: return "Ambigious command";
    case (int) E_SYNTAX:        return "Syntax error";
    case (int) E_ERROR:         return "Error";
    default:                    return strerror(e);
  }
}

int error_get_errno() {
  return this.error_no;
}

void error_free() {
  if (this.cause)
    free(this.cause);

  for (int i = 0; i < this.additional_msgs_sz; ++i)
    free(this.additional_msgs[i]);

  if (this.error_msg)
    free(this.error_msg);

  /* this = (struct errors_t) {
      .error_msg          = NULL,
      .cause              = NULL,
      .additional_msgs_sz = 0,
      .error_no           = 0
  }; */
  memset(&this, 0, sizeof(this));
}

// Set error number only
void error_set_errno(int number) {
  error_free();
  this.error_no = number;
}

// Set error and cause
void error_set(int number, const char *cause) {
  error_free();
  this.error_no = number;
  this.cause    = strdup(cause);
}

#if 0
// Set error and cause (printf-like)
void error_setf(int number, const char *fmt, ...) {
  char buf[ERROR_MESSAGE_MAX_LEN];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  error_set(number, buf);
}
#endif

// Add error message
void error_add(const char *message) {
  if (this.additional_msgs_sz >= ERROR_ADDITIONAL_MESSAGES_MAX)
    this.additional_msgs_sz--; // overwrite last message
  this.additional_msgs[this.additional_msgs_sz++] = strdup(message);
}

#if 0
// Add error message (printf-like)
void error_addf(const char *fmt, ...) {
  char buf[ERROR_MESSAGE_MAX_LEN];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  error_add(buf);
}
#endif

// Return full error message
#define ERROR_SEP     ": "
#define ERROR_SEP_LEN (sizeof(ERROR_SEP) - 1)

char* error_get() {
  char buf[1 + (1 + ERROR_MESSAGE_MAX_LEN) * (ERROR_ADDITIONAL_MESSAGES_MAX + ERROR_SEP_LEN)] = { 0 };

  for (int i = this.additional_msgs_sz; i--;)
    if (this.additional_msgs[i]) {
      strcat(buf, this.additional_msgs[i]);
      strcat(buf, ERROR_SEP);
    }

  strcat(buf, error_strerror(this.error_no));
  if (this.cause) {
    strcat(buf, ERROR_SEP);
    strcat(buf, this.cause);
  }

  if (this.error_msg)
    free(this.error_msg);
  this.error_msg = strdup(buf);
  return this.error_msg;
}
