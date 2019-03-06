#include "tkremap.h"

static COMMAND_CALL_FUNC(cmd_unbind_call) {
  int argc;
  char **args;
  unpackargs(&argc, &args, NULL, (command_args_t*) cmd->arg);

  TermKeyKey tkey;
  binding_t *binding      = context.current_mode->root,
            *binding_next = NULL,
            *binding_last = NULL,

  for (int i = 0; i < argc; ++i) {
    if (! parse_key(args[i], &tkey))
      return 0;

    binding_last = binding;
    binding_next = binding_get_binding(binding, &tkey);

    if (! binding_next)
      return 1; // binding not available
    else
      binding = binding_next;
  }

  if (binding_last)
    binding_del_binding(binding_last, &tkey);
  return 1;
}

command_t command_unbind = {
  .name  = "unbind",
  .desc  = "Unbind key",
  .args  = (const char*[]) { "+KEY", 0 },
  .parse = &copyargs,
  .free  = &deleteargs,
  .call  = &cmd_unbind_call
};

