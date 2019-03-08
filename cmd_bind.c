#include "tkremap.h"
#include "commands.h"

#ifndef CMD_BIND_MAX_KEYS
#define CMD_BIND_MAX_KEYS 256
#endif

static COMMAND_CALL_FUNC(cmd_bind_call) {
  int argc;
  char **args;
  unpackargs(&argc, &args, NULL, (command_args_t*) cmd->arg);

  int i,     keys_size = 0;
  TermKeyKey keys[CMD_BIND_MAX_KEYS];
  commands_t *commands;
  binding_t  *binding = context.current_mode->root,
             *next;

  // Read all keys first
  for (i = 0; i < argc - 1; ++i) {
    if (keys_size >= CMD_BIND_MAX_KEYS)
      break;
    if (! parse_key(args[i], &keys[i]))
      break;
    keys_size++;
  }

  if (! keys_size)
    return error_write("%s: %s", E_MISSING_ARG, "KEY"), 0;

  // Build our command
  args = &args[i];
  argc = argc - i;

  if (argc == 0)
    return error_write("%s: %s", E_MISSING_ARG, "COMMAND"), 0;

  if (! (commands = commands_parse(argc, args)))
    return 0;

  // Go through keybindings, create bindings if needed, insert command
  for (i = 0; i < keys_size; ++i) {
    next = binding_get_binding(binding, &keys[i]);

    if (! next) {
      next      = calloc(1, sizeof(binding_t));
      next->key = keys[i];
      binding   = binding_add_binding(binding, next);
    }
    else
      binding = next;
  }

  // Overwrite old command
  if (binding->commands) {
    commands_free(binding->commands);
    free(binding->commands);
  }

  binding->commands = commands;
  return 1;
}

command_t command_bind = {
  COMMAND_T_INIT,
  .name  = "bind",
  .desc  = 
    "Bind _KEY_ to _COMMAND_\n"
    "Multiple commands can be specified, they have to be seperated by '\\\\;' or '&&'.\n"
    "Keys can be chained.",
  .args  = (const char*[]) { "+KEY", "+COMMAND", 0 },
  .call  = &cmd_bind_call,
  .parse = &copyargs,
  .free  = &deleteargs
};

