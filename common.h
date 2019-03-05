#ifndef _COMMON_H
#define _COMMON_H

#include <stdlib.h>
#include <string.h>

#define streq(a,b) \
  (!strcmp(a,b))

#define strcaseq(a,b) \
  (!strcasecmp(a,b))

#define freeArray(array, n) \
  do { \
    for (int __I = 0; __I < n; ++__I) \
    free(array[__I]); \
    free(array); \
  } while(0)

#define freeArrayNULL(array) \
  do { \
    for (int __I = 0; array[__I]; ++__I) \
    free(array[__I]); \
    free(array); \
  } while(0)

#define dumpArray(array, n) \
  do { \
    printf("["); \
    if (n > 0) \
    printf("'%s'", array[0]); \
    for (int __I = 1; __I < n; ++__I) \
    printf(",'%s'", array[__I]); \
    printf("]\n"); \
  } while(0)

#define MEMDUP(PTR) \
  memdup(PTR, sizeof(*PTR));

inline __attribute__((always_inline))
  int strprefix(const char *string, const char *prefix) {
    return (strstr(string, prefix) == string);
  }

inline __attribute__((always_inline))
  void* memdup(void *src, int size) {
    void *dest = malloc(size);
    return memcpy(dest, src, size);
  }

#endif
