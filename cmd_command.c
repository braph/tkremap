#define _GNU_SOURCE
#include <stdio.h>
#include "tkremap.h"

typedef struct cmd_readline_args { // cmd_readline.c + cmd_command.c
  uint8_t do_newline       : 1;
  uint8_t do_clear         : 1;
  uint8_t do_refresh       : 1;
  uint8_t do_config        : 1;
  uint8_t do_savescreen    : 1;
  uint8_t is_redisplayed   : 1;
  uint8_t __PAD__          : 2;
  int16_t x;
  int16_t y;
  char  *prompt;
  char  *init;
  char  *keyseq;
  char  *prepend;
  char  *append;
} cmd_readline_args;

command_t command_readline;
void  cmd_readline_free(void *);
int   cmd_readline_call(struct command_call_t*, TermKeyKey *);
void* cmd_readline_parse(int, char *[], option *);

static COMMAND_PARSE_FUNC(cmd_command_parse) {
  cmd_readline_args *cmd_args = cmd_readline_parse(argc, args, options);
  if (cmd_args) {
    cmd_args->do_config = 1;

    if (cmd_args->prompt) {
      char *old_prompt = cmd_args->prompt;
      asprintf(&cmd_args->prompt, "(mode: %s) %s", context.current_mode->name, old_prompt);
      free(old_prompt);
    }
    else
      asprintf(&cmd_args->prompt, "(mode: %s) ", context.current_mode->name);
  }
  return (void*) cmd_args;
}

command_t command_command = {
  .name  = "command",
  .desc  = 
    "Read and execute a tkremap command",
  .opts  = (const command_opt_t[]) {
    {'p', "PROMPT", "See *readline*"},
    {'i', "TEXT",   "See *readline*"},
    {'x', "X",      "See *readline*"},
    {'y', "Y",      "See *readline*"},
  //{'b', NULL,     "See *readline*"},
    {'n', NULL,     "See *readline*"},
    {'k', "KEY",    "See *readline*"},
    {'c', NULL,     "See *readline*"},
    {'r', NULL,     "See *readline*"},
    {'s', NULL,     "See *readline*"},
    {'A', "TEXT",   "See *readline*"},
    {'P', "TEXT",   "See *readline*"},
    {0,0,0}
  },
  .parse = &cmd_command_parse,
  .call  = &cmd_readline_call,
  .free  = &cmd_readline_free
};

