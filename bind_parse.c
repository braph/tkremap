#include "tkremap.h"
#include "common.h"
#include "commands.h"
#include "errormsg.h"

/* parse single command, append to binding */
static
int binding_append_command(binding_t *binding, int argc, char *args[])
{
  command_t *cmd = NULL;
  void      *arg = NULL;

  if (! (cmd = get_command(args[0])))
    return 0;

  if (! (arg = command_parse(cmd, argc - 1, &args[1])))
    return error_add("%s", cmd->name), 0;

  binding->p.commands = realloc(binding->p.commands, ++binding->size * sizeof(command_call_t));
  binding->p.commands[binding->size - 1].command = cmd;
  binding->p.commands[binding->size - 1].arg = arg;
  return 1;
}

static
void binding_append_command_separator(binding_t *binding, int command_separator) {
  binding->p.commands = realloc(binding->p.commands, ++binding->size * sizeof(command_call_t));
  binding->p.commands[binding->size - 1].command = (command_t*) (uintptr_t) command_separator;
}

// TODO: free if failed
/* parse multiple commands, append to binding */
int binding_append_commands(binding_t *binding, int argc, char *args[])
{
  int command_separator = -1;
  int j;

  for (int i = 0; i < argc; ++i) {
    for (j = i + 1; j < argc; ++j) {
      command_separator = -1;
      if (streq(args[j], "\\;")) {
        command_separator = COMMAND_SEPARATOR_SEMICOLON;
        break;
      }
      if (streq(args[j], "&&")) {
        command_separator = COMMAND_SEPARATOR_AND;
        break;
      }
    }

    if (! binding_append_command(binding, j - i, &args[i]))
      return 0;
    i = j;

    if (command_separator == COMMAND_SEPARATOR_SEMICOLON ||
        command_separator == COMMAND_SEPARATOR_AND)
        binding_append_command_separator(binding, command_separator);
  }

  return 1;
}
