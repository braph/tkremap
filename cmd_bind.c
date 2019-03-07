#include "tkremap.h"

// bind_parse.c
commands_t* commands_parse(int, char *[]);

static COMMAND_CALL_FUNC(cmd_bind_call) {
  int argc;
  char **args;
  unpackargs(&argc, &args, NULL, (command_args_t*) cmd->arg);

  TermKeyKey tkey;
  binding_t  *binding, *binding_next;

  binding = context.current_mode->root;

  // read till last keydef
  while (argc > 1 && parse_key(args[0], &tkey)) {
    args_get_arg(&argc, &args, NULL);
    binding_next = binding_get_binding(binding, &tkey);

    if (! binding_next) {
      binding_next      = calloc(1, sizeof(binding_t));
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

  binding->commands = commands_parse(argc, args);
  return 1; // TODO
}

command_t command_bind = {
  COMMAND_T_INIT,
  .name  = "bind",
  .desc  = 
    "Bind _KEY_ to _COMMAND_\n"
    "Multiple commands can be specified, they have to be seperated by '\\\\;'.\n"
    "Keys can be chained",
  .args  = (const char*[]) { "+KEY", "+COMMAND", 0 },
  .opts  = NULL,
  .call  = &cmd_bind_call,
  .parse = &copyargs,
  .free  = &deleteargs
};

