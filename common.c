#include "common.h"

#include <unistd.h>
#include <errno.h>

/* Immutable array
 *
 * Returns a duplicate of *args[] in one alloc.
 * 
 * [ array ][      data         ]
 * [1][2][3][1...][2.....][3....]
 *  `--|--|--'     |       |
 *     `--|--------'       |
 *        `----------------'
 */
char** immutable_array(int argc, char *args[]) {
  int psize = (argc + 1) * sizeof(char*);
  int dsize = (argc + 1);
  for (int i = argc; i--;)
    dsize += strlen(args[i]);

  union immu_array_t {
    char *array[1];
    char   data[1];
  };

  union immu_array_t *immutable = malloc(psize + dsize);
  if (! immutable)
    return NULL;

  immutable->array[argc] = NULL;
  char *data = immutable->data + psize;
  for (int i = 0; i < argc; ++i) {
    strcpy(data, args[i]);
    immutable->array[i] = data;
    data += strlen(args[i]) + 1;
  }

  return immutable->array;
}

int strprefix(const char *string, const char *prefix) {
  if (! *prefix) // do not match empty prefix!
    return 0;

  while (*prefix)
    if (*prefix++ != *string++)
      return 0;
  return 1;
}

int strsuffix(const char *string, const char *suffix) {
  int str_len = strlen(string);
  int suf_len = strlen(suffix);
  int offset  = str_len - suf_len;
  if (offset < 0 || !streq(string + offset, suffix))
    return 0;
  return 1;
}

void write_full(int fd, const char *s, ssize_t len) {
  ssize_t n;

  for (int i = 5; i--; ) {
    n = write(fd, s, len);
    if (n >= 0) {
      s += n;
      len -= n;
      if (len == 0)
        break;
    } else if (n == -1 && errno != EAGAIN)
      break;
    usleep(100);
  }
}

