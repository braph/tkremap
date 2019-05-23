



#define MAX 2048
void error_add(const char *fmt, ...) {
  char *olderror = error_msg;
  char temp[MAX];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(temp, MAX, fmt, ap);
  va_end(ap);

  debug("created: %s", temp);

  error_sz = asprintf(&error_msg, "%s: %s", temp, olderror) + 1;
  free(olderror);
}
