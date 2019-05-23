// version 1:
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

// version 2:
static int lex_args(char ***args, int *n) {
  int ttype;
  int slen, total_len = 0;
  char *s;

  *n = 0;
  if (*args == NULL) {
    *args      = malloc(MAX_ARGS * sizeof(char*));
    *(args)[0] = malloc(MAX_TOTAL_SIZE);
  }

  s = (*args)[0];

  for (;;) {
    ttype = lex_lex();
    if (ttype == LEX_TOKEN_END) {
      if (*n > 0)
        break;
      else
        continue;
    }

    if (ttype < 0) {
      if (ttype == EOF)
        break;
      error_setf(E_SYNTAX, "on line %d:%d: %s (%s)", lex_line_no(), lex_char_pos(), lex_error(), lex_line());
      return EOF;
    }

    if (*n >= MAX_ARGS)
      return error_setf(E2BIG, "max arguments is %d", MAX_ARGS), -1;

    total_len += (slen = strlen(lex_token()));
    if (total_len >= MAX_TOTAL_SIZE)
      return error_setf(E2BIG, "Sum of arguments length exceeded max size (%d)", MAX_TOTAL_SIZE), -1;

    ++(*n);
    strcpy(s, lex_token());
    (*args)[*n - 1] = s;
    s += (slen + 1);
  }

  return *n;
}

// === old read_conf* functions ===============================================

int read_conf_stream(FILE *fh) {
  int             ret   = 1;
  int             nargs = 0;
  char          **args  = NULL;
  command_call_t cmdcall;
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

    if (! command_parse(nargs, args, &cmdcall)) {
      error_addf("on line %d:%d", lex_line_no(), lex_char_pos());
      ret = 0;
      break;
    }

    ret = cmdcall.command->call(&cmdcall, NULL);
    command_call_free(&cmdcall);
    if (! ret) {
      error_addf("on line %d:%d", lex_line_no(), lex_char_pos());
      break;
    }
  }

//END
  if (args) {
    free(args[0]);
    free(args);
  }
  lex_destroy();
  context.current_mode = mode_restore;
  return ret;
}

int read_conf_string(const char *str) {
  FILE *fh = fmemopen((void*) str, strlen(str), "r");

  if (! fh)
    return error_set_errno(errno), 0;

  int ret = read_conf_stream(fh);
  fclose(fh);
  return ret;
}

int read_conf_file(const char *file) {
  FILE *fh = fopen(file, "r");

  if (! fh)
    return error_set_errno(errno), 0;

  int ret = read_conf_stream(fh);
  fclose(fh);
  return ret;
}
