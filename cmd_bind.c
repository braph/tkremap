#include "tkremap.h"

// bind_parse.c
int binding_append_commands(binding_t*, int, char *[]);


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
      binding_next             = malloc(sizeof(binding_t));
      binding_next->key        = tkey;
      binding_next->size       = 0;
      binding_next->p.commands = NULL;
      binding_next->type       = BINDING_TYPE_CHAINED;
      binding = binding_add_binding(binding, binding_next);
    }
    else {
      if (binding_next->type == BINDING_TYPE_COMMAND)
        return error_write("Overwriting key binding"), 0; // TODO

      binding = binding_next;
    }
  }

  if (binding == context.current_mode->root)
    return error_write("%s: %s", E_MISSING_ARG, "KEY"), 0;

  if (binding->type == BINDING_TYPE_COMMAND)
    return error_write("Overwriting key binding"), 0;

  if (binding->type == BINDING_TYPE_CHAINED && binding->size > 0)
    return error_write("Overwriting key binding"), 0;

  if (argc == 0)
    return error_write("%s: %s", E_MISSING_ARG, "COMMAND"), 0;

  binding->type = BINDING_TYPE_COMMAND;
  return binding_append_commands(binding, argc, args);
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

