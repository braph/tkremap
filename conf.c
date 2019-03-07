#include "conf.h"
#include "lexer.h"
#include "common.h"
#include "tkremap.h"
#include "commands.h"
#include "errormsg.h"
#include "termkeystuff.h"

#include <string.h>

static int lex_args(char ***args, int *n) {
  int ttype;
  int s = *n;

  while (*n)
    free((*args)[--(*n)]);

  for (;;) {
    ttype = lex_lex();
    if (ttype == LEX_TOKEN_END)
      break;

    if (ttype == EOF) {
      if (lex_error_num) {
        error_write("%s", lex_error());
        while (*n)
          free((*args)[--(*n)]);
        free(*args);
        *args = NULL;
        return -1;
      }
      break;
    }

    ++*n;
    if (*n >= s) {
      s = *n + 32;
      *args = realloc(*args, s * sizeof(char*));
    }

    (*args)[*n - 1] = strdup(lex_token());
  }

  return *n;
}

int read_conf_stream(FILE *fh) {
  int             ret = 1;
  int             nargs = 0;
  char          **args = NULL;
  command_call_t *cmdcall = malloc(sizeof(command_call_t));
  keymode_t      *mode_restore = context.current_mode;

  lex_init(fh);

  while (! lex_eof()) {
    int lret = lex_args(&args, &nargs);
    if (lret == -1) {
      ret = 0;
      break;
    }
    if (lret == 0)
      break;

    cmdcall = command_parse(nargs, args, cmdcall);
    if (! cmdcall) {
      error_add("%d:%d", lex_line, lex_line_pos);
      ret = 0;
      break;
    }

    ret = cmdcall->command->call(cmdcall, NULL);
    command_call_free(cmdcall);
    if (! ret) {
      error_add("%d:%d", lex_line, lex_line_pos);
      break;
    }
  }

//END
  free(cmdcall);
  freeArray(args, nargs);
  lex_destroy();
  context.current_mode = mode_restore;
  return ret;
}

int read_conf_string(const char *str) {
  FILE *fh = fmemopen((void*) str, strlen(str), "r");

  if (! fh)
    return error_write("%s", strerror(errno)), 0;

  int ret = read_conf_stream(fh);
  fclose(fh);
  return ret;
}

int read_conf_file(const char *file) {
  FILE *fh = fopen(file, "r");

  if (! fh)
    return error_write("%s", strerror(errno)), 0;

  int ret = read_conf_stream(fh);
  fclose(fh);
  return ret;
}
