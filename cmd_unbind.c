#include "tkremap.h"

#ifndef CMD_UNBIND_MAX_KEYS
#define CMD_UNBIND_MAX_KEYS 1024
#endif

void binding_del_binding(binding_t *binding, TermKeyKey *key) {
  for (int i=0; i < binding->size; ++i)
    if (! termkey_keycmp(tk, key, &binding->bindings[i]->key)) {
      free(binding->bindings[i]);
      for (++i; i < binding->size; ++i)
        binding->bindings[i - 1] = binding->bindings[i];
      break;
    }

  binding->size--;
  binding->bindings = realloc(binding->bindings, binding->size * sizeof(binding_t*));
}

typedef struct cmd_unbind_args_t {
  int        size;
  TermKeyKey keys[0];
} cmd_unbind_args_t;

static COMMAND_PARSE_FUNC(parse) {
  if (argc > CMD_UNBIND_MAX_KEYS)
    return error_set_errno(E2BIG), NULL;

  cmd_unbind_args_t *cmd_args =
    malloc(sizeof(cmd_unbind_args_t) + (argc * sizeof(TermKeyKey)));

  for (int i = -1; ++i < argc;) {
    if (! parse_key(args[i], &cmd_args->keys[i])) {
      free(cmd_args);
      return error_add(args[i]), NULL;
    }
  }

  return cmd_args;
}

static COMMAND_CALL_FUNC(call) {
  cmd_unbind_args_t *cmd_args = (cmd_unbind_args_t*) cmd->arg;

  int i;
  binding_t *binding_stack[CMD_UNBIND_MAX_KEYS];
  binding_t *binding = context.current_mode->root;

  for (i = -1; ++i < cmd_args->size;) {
    binding = binding_get_binding(binding, &cmd_args->keys[i]);

    if (! binding)
      return 1; // Binding not available
    else
      binding_stack[i] = binding;
  }

  // Remove command from binding
  --i;
  if (binding_stack[i]->command != NULL && PTR_UNREF(binding_stack[i]->command)) {
    command_call_free(binding_stack[i]->command);
    free(binding_stack[i]->command);
    binding_stack[i]->command = NULL;
  }

  while (--i >= 0) {
    binding_del_binding(binding_stack[i], &binding_stack[i+1]->key);

    if (binding_stack[i]->size > 0)
      break;
  }

  return 1;
}

const command_t command_unbind = {
  .name  = "unbind"
    "\0Unbind _KEYs_",
  .args  = "+KEYs\0",
  .parse = &parse,
  .call  = &call,
  .free  = &free
};

