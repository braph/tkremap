#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0 // TCC
void * __dso_handle __attribute((visibility("hidden"))) = &__dso_handle;
#endif

//#define snprintf(...) snprintf_repl(__VA_ARGS__)
//#define printf(...) (0)
//#define printf(...) printf_repl(__VA_ARGS__)

// === debugging stuff ========================================================
#if DEBUG
#include <assert.h>

#define debug(FMT, ...) \
  fprintf(stderr, __FILE__ ": " FMT "\n", ##__VA_ARGS__);

#define ___(FMT, ...) \
  debug(FMT "()", ##__VA_ARGS__)

#define debugArray(array, n) \
  do { \
    printf("["); \
    if (n > 0) \
      printf("'%s'", array[0]); \
    for (int __I = 1; __I < n; ++__I) \
      printf(",'%s'", array[__I]); \
    printf("]\n"); \
  } while(0)

#else
#define DEBUG             0
#define ___(...)          NULL
#define assert(...)       NULL
#define debug(...)        NULL
#define debugArray(...)   NULL
#endif

#ifndef TIOCGWINSZ
#define TIOCGWINSZ  0x5413
#endif

#ifndef TIOCSWINSZ
#define TIOCSWINSZ  0x5414
#endif

// === reference management ===================================================
// Use this to "duplicate" an object
#define PTR_REF(OBJECT) \
  (++(OBJECT->refcount), OBJECT)

// Use this to unreference an object. Returns TRUE if object can be free'd
#define PTR_UNREF(OBJECT) \
  (! --(OBJECT->refcount))

// === compiler attributes ====================================================
#define __packed \
  __attribute__((__packed__)) 

#define __inline \
  __attribute__((always_inline))

#define __fallthrough \
  __attribute__((fallthrough))

// === string comparision macros ==============================================
#define streq(a,b) \
  (!strcmp(a,b))

#define strcaseq(a,b) \
  (!strcasecmp(a,b))

#define STRLEN(S) \
  (sizeof("" S) - 1)

// === i/o ====================================================================
#define bufprintf(BUFFER, ...) \
  snprintf(BUFFER, sizeof(BUFFER), __VA_ARGS__)

// === free macros ============================================================
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

#define MEMDUP(PTR) \
  memdup(PTR, sizeof(*PTR))

#define memdup(SRC, SIZE) \
  memcpy(malloc(SIZE), SRC, SIZE)

#if 0
static void* __inline memdup(void *src, int size) {
  void *dest = malloc(size);
  if (dest)
    memcpy(dest, src, size);
  return dest;
}
#endif

// === terminal stuff =========================================================
#define SMCUP      "\033[?1049h" // tput smcup
#define RMCUP      "\033[?1049l" // tput rmcup
#define CLEAREOL   "\033[K"      // tput el
#define CLEARBOL   "\033[1K"     // tput el1

#define SAVE_TERM_SCREEN(FD) \
  write(FD, SMCUP, STRLEN(SMCUP))

#define RESTORE_TERM_SCREEN(FD) \
  write(FD, RMCUP, STRLEN(RMCUP))

#define CLEAR_TO_EOL(FD) \
  write(FD, CLEAREOL, STRLEN(CLEAREOL))

#define CLEAR_TO_BOL(FD) \
  write(FD, CLEARBOL, STRLEN(CLEARBOL))

// === misc ===================================================================
#define _HELPER_(S) #S
#define TO_STR(S)  _HELPER_(S)

#define SWAP(A, B) \
  (A ^= B, B ^= A, A ^= B)

void    write_full(int, const char*, ssize_t);
int     strprefix(const char *string, const char *prefix);
int     strsuffix(const char *string, const char *suffix);
char**  immutable_array(int argc, char *args[]);

int     snprintf_repl(char*, size_t, const char*, ...);
int     printf_repl(const char*, ...);

char*   strdup_const(const char*);
void    free_const(void*);

#endif
