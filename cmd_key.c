#include "tkremap.h"

#include <string.h>
#include <stdint.h>
#include <unistd.h>

typedef struct __packed cmd_key_args {
  uint16_t repeat;
  char     string[1];
} cmd_key_args;

static COMMAND_CALL_FUNC(call) {
  cmd_key_args *arg = (cmd_key_args*) cmd->arg;

  for (int i = arg->repeat; i--;)
    writes_to_program(arg->string);

  return 1;
}

static COMMAND_PARSE_FUNC(parse) {
  const char *seq;
  int         repeat = 1;
  char        sequences[argc * 16];

  for (option *opt = options; opt->opt; ++opt) {
    if (opt->opt == 'r')
      if ((repeat = atoi(opt->arg)) <= 0)
        return error_set(EINVAL, opt->arg), NULL;
  }

  sequences[0] = 0;
  for (int i = 0; i < argc; ++i) {
    if (! (seq = key_parse_get_code(args[i])))
      return error_add(args[i]), NULL;
    strcat(sequences, seq);
  }

  cmd_key_args *cmd_args = malloc(sizeof(*cmd_args) + strlen(sequences));
  cmd_args->repeat = repeat;
  strcpy(cmd_args->string, sequences);

  return (void*) cmd_args;
}

const command_t command_key = {
  .name  = "key"
    "\0Send keys to program",
  .args  = "+KEY\0",
  .opts  = OPTIONS(
    OPTION('r', "N", "Repeat the key N times")
  ),
  .parse = &parse,
  .call  = &call,
  .free  = &free
};

