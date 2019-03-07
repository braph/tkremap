#include <stdlib.h>
#include <string.h>

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
