#define _GNU_SOURCE
#include <stdio.h>
#include "tkremap.h"

typedef struct cmd_readline_args { // cmd_readline.c + cmd_command.c
  uint8_t  do_newline : 1;
  uint8_t  do_clear   : 1;
  uint8_t  do_refresh : 1;
  uint8_t  do_config  : 1;
  uint8_t  __PAD__    : 4;
  int16_t x;
  int16_t y;
  char  *prompt;
  char  *init;
  char  *keyseq;
  char  *prepend;
  char  *append;
} cmd_readline_args;

const command_t command_readline;
void  cmd_readline_free(void *_arg);
int   cmd_readline_call(struct command_call_t* cmd, TermKeyKey *key);
void* cmd_readline_parse(int argc, char *args[], option *options);

static COMMAND_PARSE_FUNC(cmd_command_parse) {
  cmd_readline_args *cmd_args = cmd_readline_parse(argc, args, options);
  if (cmd_args) {
    cmd_args->do_config = 1;

    if (cmd_args->prompt) {
      char *old_prompt = cmd_args->prompt;
      asprintf(&cmd_args->prompt, "(mode: %s) %s", context.current_mode->name, old_prompt);
      free(old_prompt);
    }
    else {
      asprintf(&cmd_args->prompt, "(mode: %s) ", context.current_mode->name);
    }
  }
  return (void*) cmd_args;
}

const command_t command_command = {
  .name  = "command",
  .desc  = 
    "Read and execute a configuration string\n\n"
    "See *readline* for a description of options.\n",
  .opts  = (const command_opt_t[]) {
    {'p', "PROMPT", "Set prompt text"},
    {'i', "INIT",   "Pre fill buffer with text"},
    {'x', "X",      "Set x cursor position (starting from left - use negative value to count from right)"},
    {'y', "Y",      "Set y cursor position (starting from top - use negative value to count from bottom)"},
    {'n', NULL,     "Append a newline to result string"},
    {'k', "KEY",    "Send _KEY_ after writing line"},
    {'c', NULL,     "Clear the cursor line"},
    {'r', NULL,     "Refresh the window (*1*)"},
    {'P', "TEXT",   "Prepend _TEXT_ to result"},
    {'A', "TEXT",   "Append _TEXT_ to result"},
    {'C', NULL,     "Evaluate result as config instead of sending it to program"},
    {0,0,0}
  },
  .parse = &cmd_command_parse,
  .call  = &cmd_readline_call,
  .free  = &cmd_readline_free
};

