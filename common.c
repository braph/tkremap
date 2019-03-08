#include <stdlib.h>
#include <string.h>

#if 0
char** charsdup(int argc, char *args[]) {
  char **args2 = malloc((argc + 1) * sizeof(char*));

  if (args2) {
    for (int i = 0; i < argc; ++i)
      if (! (args2[i] = strdup(args[i]))) {
        while (--i >= 0)
          free(args2[i]);
        free(args2);
        return NULL;
      }

    args2[argc] = NULL;
  }

  return args2;
}
#endif

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

