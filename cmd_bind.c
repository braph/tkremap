#include "tkremap.h"
#include "commands.h"
#include "lexer.h"

#ifndef CMD_BIND_MAX_KEYS
#define CMD_BIND_MAX_KEYS 256
#endif

typedef struct cmd_bind_args_t {
  command_call_t  *command_call;
  int              keys_size;
  TermKeyKey       keys[0]; // bottom, important!
} cmd_bind_args_t;

static COMMAND_PARSE_FUNC(parse) {
  ___("bind-parse");
  int keys_size = 0;
  TermKeyKey keys[CMD_BIND_MAX_KEYS];
  command_call_t *command_call;

  while (1) {
    int ttype = lex_lex();
    debug("bind(): ttype = %s", LEX_TOK2STR(ttype));

    if (ttype != LEX_TOKEN_WORD) {
      lex_unlex();
      break;
    }

    const char *keydef = lex_token();
    debug("bind(): keydef = %s", keydef);

    if (keys_size == CMD_BIND_MAX_KEYS) {
      return error_set_errno(E2BIG), NULL;
    }

    if (! parse_key(keydef, &keys[keys_size])) {
      lex_unlex();
      break;
    }

    ++keys_size;
  }

  debug("bind(): having %d keys", keys_size);

  if (! keys_size)
    return error_set(E_MISSING_ARG, "KEY"), NULL;

  // Build our command (TODO: handle missing command?)
  if (! (command_call = command_parse(NULL, NULL))) {
    debug("HEHEHE");
    return NULL;
  }

  #define SIZE (keys_size * sizeof(TermKeyKey))
  cmd_bind_args_t *cmd_args = malloc(sizeof(cmd_bind_args_t) + SIZE);
  cmd_args->command_call    = PTR_REF(command_call);
  cmd_args->keys_size       = keys_size;
  memcpy(cmd_args->keys, &keys, SIZE);
  return cmd_args;
}

static COMMAND_CALL_FUNC(call) {
  cmd_bind_args_t *cmd_args = (cmd_bind_args_t*) cmd->arg;
  binding_t       *binding  = context.current_mode->root,
                  *next;

  // Go through keybindings, create bindings if needed, insert command
  for (int i = 0; i < cmd_args->keys_size; ++i) {
    next = binding_get_binding(binding, &cmd_args->keys[i]);

    if (! next) {
      next      = calloc(1, sizeof(binding_t));
      next->key = cmd_args->keys[i];
      binding   = binding_add_binding(binding, next);
    }
    else
      binding = next;
  }

  // Overwrite old command
  if (binding->command && PTR_UNREF(binding->command)) {
    command_call_free(binding->command);
    free(binding->command);
  }

  binding->command = PTR_REF(cmd_args->command_call);
  return 1;
}

static void cmd_bind_free(void *args) {
  cmd_bind_args_t *cmd_args = (cmd_bind_args_t*) args;

  if (PTR_UNREF(cmd_args->command_call)) {
    command_call_free(cmd_args->command_call);
    free(cmd_args->command_call);
  }

  free(args);
}

const command_t command_bind = {
  .name  = "bind"
    "\0Bind _KEYs_ to _COMMAND_\n"
    "Use a command group '*{*' for passing multiple commands",
  .args  = "+KEYs\0COMMAND\0",
  .call  = &call,
  .parse = &parse,
  .free  = &cmd_bind_free
};

