#include "tkremap.h"

void binding_del_binding(binding_t *binding, TermKeyKey *key) {
  for (int i=0; i < binding->size; ++i)
    if (! termkey_keycmp(tk, key, &binding->bindings[i]->key)) {
      //binding_free(binding->bindings[i]);
      for (++i; i < binding->size; ++i)
        binding->bindings[i - 1] = binding->bindings[i];
      break;
    }

  binding->size--;
  binding->bindings = realloc(binding->bindings, binding->size * sizeof(binding_t*));
}

static COMMAND_CALL_FUNC(cmd_unbind_call) {
  int argc;
  char **args;
  unpackargs(&argc, &args, NULL, (command_args_t*) cmd->arg);

  int i;
  TermKeyKey tkey;
  binding_t *binding_stack[128];
  binding_t *binding = context.current_mode->root;

  for (i = 0; i < argc; ++i) {
    if (! parse_key(args[i], &tkey))
      return 0;

    binding = binding_get_binding(binding, &tkey);

    if (! binding)
      return 1; // binding not available
    else
      binding_stack[i] = binding;
  }

  // remove command from binding
  --i;
  if (binding_stack[i]->commands != NULL) {
    commands_free(binding_stack[i]->commands);
    free(binding_stack[i]->commands);
    binding_stack[i]->commands = NULL;
  }

  while (--i >= 0) {
    binding_del_binding(binding_stack[i], &binding_stack[i+1]->key);

    if (binding_stack[i]->size > 0)
      break;
  }

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

