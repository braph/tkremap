
#include "tkremap.h"
#include "commands.h"

#ifndef CMD_BIND_MAX_KEYS
#define CMD_BIND_MAX_KEYS 256
#endif

typedef struct cmd_bind_args_t {
  commands_t  *commands;
  int          keys_size;
  TermKeyKey   keys[CMD_BIND_MAX_KEYS]; // on bottom, important!
} cmd_bind_args_t;

static COMMAND_PARSE_FUNC(cmd_bind_parse) {
  int i;
  cmd_bind_args_t bind_args;

  // Read all keys first
  for (i = 0; i < argc - 1; ++i) {
    if (bind_args.keys_size >= CMD_BIND_MAX_KEYS)
      break; // TODO: error?
    if (! parse_key(args[i], &bind_args.keys[i]))
      break;
    bind_args.keys_size++;
  }

  if (! bind_args.keys_size) {
    error_set(E_MISSING_ARG, "KEY");
    return 0;
  }

  // Build our command
  args = &args[i];
  argc = argc - i;

  if (argc == 0)
    return error_set(E_MISSING_ARG, "COMMAND"), 0;

  if (! (bind_args.commands = commands_parse(argc, args)))
    return 0;

  // Save some bytes
  int actual_size = sizeof(bind_args) - ((CMD_BIND_MAX_KEYS - bind_args.keys_size) * sizeof(TermKeyKey))
  cmd_bind_args_t *copy = malloc(actual_size);
  memcpy(copy, &bind_args, actual_size);
  return copy;
}

static COMMAND_CALL_FUNC(cmd_bind_call) {
  cmd_bind_args *cmd_args = (cmd_bind_args*) cmd->arg;
  binding_t     *binding = context.current_mode->root,
                *next;

  // Go through keybindings, create bindings if needed, insert command
  for (int i = 0; i < cmd_args->keys_size; ++i) {
    next = binding_get_binding(binding, &cmd_args->keys[i]);

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
    // No need for freeing, as current commands are the new commands
    // In fact, freeing would corrupt ...
    if (binding->commands == cmd_args->commands)
      return 0;
    commands_free(binding->commands);
    free(binding->commands);
  }

  binding->commands = commands;
  return 1;
}

command_t command_bind = {
  .name  = "bind",
  .desc  = 
    "Bind _KEY_ to _COMMAND_\n"
    "Multiple commands can be specified, they have to be seperated by '*\\\\;*' or '*&&*'.\n"
    "Keys can be chained.",
  .args  = (const char*[]) { "+KEY", "+COMMAND", 0 },
  .call  = &cmd_bind_call,
  .parse = &copyargs,
  .free  = &deleteargs
};

