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
  int slen;
  char *s;

  *n = 0;
  if (*args == NULL) {
    *args = malloc(128 * sizeof(char*));
    s     = malloc(8192);
  }
  else
    s = (*args)[0];

  for (;;) {
    ttype = lex_lex();
    if (ttype == LEX_TOKEN_END)
      break;

    if (ttype == EOF) {
      if (lex_error_num) {
        error_write("%s", lex_error());
        return -1;
      }
      break;
    }

    ++(*n);
    slen = strlen(lex_token());
    strcpy(s, lex_token());
    (*args)[*n - 1] = s;
    s += (slen + 1);
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
  free(args[0]);
  free(args);
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
