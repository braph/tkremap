#include "tkremap.h"
#include "commands.h"

static COMMAND_CALL_FUNC(cmd_bind_call) {
  int argc;
  char **args;
  unpackargs(&argc, &args, NULL, (command_args_t*) cmd->arg);

  TermKeyKey tkey;
  binding_t  *binding = context.current_mode->root,
             *binding_next;

  // read till last keydef
  while (argc > 1 && parse_key(args[0], &tkey)) {
    args_get_arg(&argc, &args, NULL);
    binding_next = binding_get_binding(binding, &tkey);

    if (! binding_next) {
      binding_next = calloc(1, sizeof(binding_t));
      binding_next->key = tkey;
      binding = binding_add_binding(binding, binding_next);
    }
    else
      binding = binding_next;
  }

  if (binding == context.current_mode->root)
    return error_write("%s: %s", E_MISSING_ARG, "KEY"), 0;

  if (argc == 0)
    return error_write("%s: %s", E_MISSING_ARG, "COMMAND"), 0;

  // overwrite
  if (binding->commands) {
    commands_free(binding->commands);
    free(binding->commands);
  }

  binding->commands = commands_parse(argc, args);
  return !! binding->commands;
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

