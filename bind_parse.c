#include "tkremap.h"
#include "common.h"
#include "commands.h"
#include "errormsg.h"

/* parse single command, append to commands */
static
int commands_append_command(commands_t *commands, int argc, char *args[])
{
  command_t *cmd = NULL;
  void      *arg = NULL;

  if (! (cmd = get_command(args[0])))
    return 0;

  if (! (arg = command_parse(cmd, argc - 1, &args[1])))
    return error_add("%s", cmd->name), 0;

  commands->commands = realloc(commands->commands, ++commands->size * sizeof(command_call_t));
  commands->commands[commands->size - 1].command = cmd;
  commands->commands[commands->size - 1].arg = arg;
  return 1;
}

static
void append_separator(commands_t *commands, int command_separator) {
  commands->commands = realloc(commands->commands, ++commands->size * sizeof(command_call_t));
  commands->commands[commands->size - 1].command = (command_t*) (uintptr_t) command_separator;
}

// TODO: free if failed
/* parse multiple commands, append to commands */
commands_t* commands_parse(int argc, char *args[])
{
  int command_separator, j;

  commands_t commands = {
    .size     = 0,
    .commands = NULL
  };

  for (int i = 0; i < argc; ++i) {
    command_separator = -1;

    for (j = i + 1; j < argc; ++j) {
      if (streq(args[j], "\\;")) {
        command_separator = COMMAND_SEPARATOR_SEMICOLON;
        break;
      }
      if (streq(args[j], "&&")) {
        command_separator = COMMAND_SEPARATOR_AND;
        break;
      }
    }

    if (! commands_append_command(&commands, j - i, &args[i]))
      return 0;
    i = j;

    if (command_separator >= 0)
        append_separator(&commands, command_separator);
  }

  return MEMDUP(&commands);
}
