#include "tkremap.h"
#include "commands.h"
#include "lexer.h"

extern command_t command_pass;

#define UNBOUND_UNICODE  (1 << TERMKEY_TYPE_UNICODE)
#define UNBOUND_KEYSYM   (1 << TERMKEY_TYPE_KEYSYM)
#define UNBOUND_FUNCTION (1 << TERMKEY_TYPE_FUNCTION)
#define UNBOUND_MOUSE    (1 << TERMKEY_TYPE_MOUSE)
#define UNBOUND_ANY      (UNBOUND_UNICODE|UNBOUND_KEYSYM|UNBOUND_FUNCTION)

typedef struct cmd_unbound_args_t {
  int             flags;
  command_call_t *command;
} cmd_unbound_args_t;

static COMMAND_PARSE_FUNC(parse) {
  // Read TYPEs
  int flags = 0;
  while (lex_lex() == LEX_TOKEN_WORD) {
    const char *arg = lex_token();
    if      (streq(arg, "any"))       flags |= UNBOUND_ANY;
    else if (streq(arg, "char"))      flags |= UNBOUND_UNICODE;
    else if (streq(arg, "sym"))       flags |= UNBOUND_KEYSYM;
    else if (streq(arg, "function"))  flags |= UNBOUND_FUNCTION;
    else if (streq(arg, "mouse"))     flags |= UNBOUND_MOUSE;
    else break;
  }
  lex_unlex();

  if (flags == 0)
    flags = UNBOUND_ANY;

  // Build COMMANDs
  command_call_t *command = command_parse(NULL, NULL);
  if (! command)
    return NULL;

  cmd_unbound_args_t *cmd_args = malloc(sizeof(cmd_unbound_args_t));
  cmd_args->flags   = flags;
  cmd_args->command = PTR_REF(command);
  return cmd_args;
}

static COMMAND_CALL_FUNC(call) {
  cmd_unbound_args_t *cmd_args = (cmd_unbound_args_t*) cmd->arg;
  keymode_t *km = context.current_mode;

  for (int f = 0; f < 4; ++f) {
    if (cmd_args->flags & (1 << f)) {
      if (km->unbound[f] && PTR_UNREF(km->unbound[f])) {
        command_call_free(km->unbound[f]);
        free(km->unbound[f]);
      }

      /* command_pass equals empty unbound[] field.
       * This saves us an alloc and is also faster in handle_key() */
      if (cmd_args->command->command == &command_pass)
        km->unbound[f] = NULL;
      else
        km->unbound[f] = PTR_REF(cmd_args->command);
    }
  }

  return 1;
}

static void cmd_unbound_free(void *args) {
  cmd_unbound_args_t *cmd_args = (cmd_unbound_args_t*) args;

  if (PTR_UNREF(cmd_args->command)) {
    command_call_free(cmd_args->command);
    free(cmd_args->command);
  }

  free(args);
}

const command_t command_unbound = {
  .name  = "unbound"
    "\0Specify action for unbound keys\n"
    "\n_TYPE_\n"
    " *char*     characters\n"
    " *sym*      symbolic keys or modified key\n"
    " *function* function keys\n"
    " *mouse*    mouse events\n"
    " *any*      char|sym|function [*default*]\n"
    "\n_COMMAND_\n"
    " Most of the time you want to use the following commands:\n\n"
    " *pass*     - for passing the key as is to the program [*this is the default*]\n"
    " *ignore*   - for completely ignoring the key\n"
    " *rehandle* - for rehandling the key (in conjunction with preceding *mode* command)\n\n"
    "Use a command group '*{*' for passing multiple commands",
  .args  = "*TYPE\0COMMAND\0",
  .parse = &parse,
  .call  = &call,
  .free  = &cmd_unbound_free
};

