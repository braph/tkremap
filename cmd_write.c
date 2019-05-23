#include "tkremap.h"

#include <string.h>
#include <unistd.h>

typedef struct __packed cmd_write_args {
  uint16_t repeat;
  char     string[1];
} cmd_write_args;

static COMMAND_CALL_FUNC(call) {
  cmd_write_args *arg = (cmd_write_args*) cmd->arg;

  for (int i = arg->repeat; i--;)
    writes_to_program(arg->string);

  return 1;
}

static COMMAND_PARSE_FUNC(parse) {
  int i;
  int size = 1;
  int repeat = 1;

  for (option *opt = options; opt->opt; ++opt) {
    if (opt->opt == 'r')
      if ((repeat = atoi(opt->arg)) <= 0)
        return error_set(EINVAL, opt->arg), NULL;
  }

  for (i = argc; i--;)
    size += strlen(args[i]);

  cmd_write_args *cmd_args = calloc(1, sizeof(*cmd_args) + size);
  cmd_args->repeat = repeat;

  for (i = 0; i < argc; ++i)
    strcat(cmd_args->string, args[i]);

  return (void*) cmd_args;
}

const command_t command_write = {
  .name  = "write"
    "\0Send string to program",
  .args  = "+STRING\0",
  .opts  = OPTIONS(
    OPTION('r', "N", "Repeat the string N times")
  ),
  .parse = &parse,
  .call  = &call,
  .free  = &free
};
