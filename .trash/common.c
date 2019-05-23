
inline __attribute__((always_inline))
int strprefix(const char *string, const char *prefix) {
  return (strstr(string, prefix) == string);
}

#define strprefix(string, prefix) \
  (strstr(string, prefix) == string)

