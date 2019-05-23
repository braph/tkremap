#include "tkremap.h"

static COMMAND_CALL_FUNC(cmd_redraw_method) {
  context.redraw_method = ((char*) cmd->arg) - 1;
  return 1;
}

#define CMD_REDRAW_NO_METHOD (char*) 9

static COMMAND_PARSE_FUNC(cmd_redraw_method_parse) {
  char *method = CMD_REDRAW_NO_METHOD;
  char sequences[argc * 16];
  const char *seq;

  for (option *opt = options; opt->opt; ++opt) {
    if (method != CMD_REDRAW_NO_METHOD)
      return error_set(E2BIG, "Please pass only one option"), NULL;

    #define case break; case
    switch (opt->opt) {
      case 'r': method = REDRAW_METHOD_RESIZE;
      case 's': method = REDRAW_METHOD_SRMCUP;
      case 'k':
        if (argc == 0)
          return error_set(E_MISSING_ARG, "Option -k wants arguments"), NULL;

        sequences[0] = 0;
        for (int i = 0; i < argc; ++i) {
          if (! (seq = key_parse_get_code(args[i])))
            return error_add(args[i]), NULL;
          strcat(sequences, seq);
        }
        method = sequences;
    }
    #undef case
  }

  if (method == CMD_REDRAW_NO_METHOD)
    return error_set(E_MISSING_ARG, "Missing option"), NULL;
  else if (method == sequences)
    method = strdup(sequences);

  return method + 1;
}

static void cmd_redraw_method_free(void *arg) {
  if ((char*) arg > REDRAW_METHOD_SRMCUP)
    free(arg);
}

const command_t command_redraw_method = {
  .name  = "redraw_method"
    "\0Set the method for redrawing the application window",
  .args  = "*KEYS\0",
  .opts  = OPTIONS(
    OPTION('k', NO_ARG, "Redraw application by sending _KEYS_.\n"
                        "Most applications recognize *C-l* for refreshing the window content"),
    OPTION('r', NO_ARG, "Redraw application by resizing the PTY [*default*]"),
    OPTION('s', NO_ARG, "Save and restore the terminal using _smcup_/_rmcup_ (see *tput*(1))")
  ),
  .call  = &cmd_redraw_method,
  .parse = &cmd_redraw_method_parse,
  .free  = &cmd_redraw_method_free
};

