#include "common.h"
#include "aliases.h"
#include "errormsg.h"
#include "commands.h"
#include "lexer.h"

extern command_t
  command_write,
  command_unbound,
  command_unbind,
  command_suspend,
  command_signal,
  command_repeat,
  command_rehandle,
  command_redraw_method,
  command_readline,
  command_pass,
  command_mode,
  command_mask,
  command_load,
  command_key,
  command_ignore,
  command_exec,
#if DEBUG
  command_debug,
#endif
  command_bind,
  command_group;

command_t* commands[] = {
  // Keep `#define COMMANDS_SIZE` (commands.h) updated!
  // List must remain sorted!
  &command_write,
  &command_unbound,
  &command_unbind,
  &command_suspend,
  &command_signal,
  &command_repeat,
  &command_rehandle,
  &command_redraw_method,
  &command_readline,
  &command_pass,
  &command_mode,
  &command_mask,
  &command_load,
  &command_key,
  &command_ignore,
  &command_exec,
#if DEBUG
  &command_debug,
#endif
  &command_bind,
  &command_group // sorting exception '{'
};

const command_t* get_command(const char *name) {
  command_t *cmd = NULL;

  for (int i = COMMANDS_SIZE; i--;)
    if (cmd) {
      if (strprefix(commands[i]->name, name))
        return error_set_errno(E_AMBIGIOUS_CMD), NULL;
      else
        break;
    }
    else if (strprefix(commands[i]->name, name)) {
      cmd = commands[i];
    }

  if (! cmd)
    error_set_errno(E_UNKNOWN_CMD);

  return cmd;
}

/* Parse a command.
 *
 * Return a `command_call_t*` type.
 *
 * A preallocated `command_call_t` can be passed using parameter `store`.
 *
 * This function expects to find a LEX_TOKEN_WORD or a LEX_TOKEN_BLOCK_BEG
 * token when calling lex_lex().
 *
 * If (a command has been found):
 *    If (command is a 'simple' command):
 *        Build argv, that is:
 *        Read all tokens until a terminator (';', '&&', '||', '}', '\n', 'EOF')
 *        was found.
 *        Invoke commands parse function
 *    If (command is a 'advanced' command):
 *        Do not build argv, instead invoke commands parse function.
 *
 * After this function returns, the next call to lex_lex() will point to
 * the terminating token.
 */

#define COMMAND_MAX_ARGS 1024

command_call_t* command_parse(void *lexer, command_call_t *store) {
  ___("command_parse");
  int     argc     = 0;
  int     argc_old = 0;
  char   *args_data[COMMAND_MAX_ARGS];
  char  **args     = args_data;
  void   *cmdarg   = NULL;
  option *options  = NULL;

  // Expect TOKEN_WORD (command name) or TOKEN_BLOCK_BEG '{' (command group)
  int ttype = lex_lex();
  if (ttype != LEX_TOKEN_WORD && ttype != LEX_TOKEN_BLOCK_BEG) {
    debug("command_parse(): first token != WORD,BLOCK_BEG is: %s", LEX_TOK2STR(ttype));
    error_set(E_SYNTAX, "Expected <COMMAND> or '{'");
    return NULL;
  }

  // Find matching command
  const char      *name = lex_token();
  const command_t *cmd  = get_command(name);
  if (! cmd) {
    debug("command_parse(): command not found: %s", name);
    error_add(name);
    return NULL;
  }
  debug("command_parse(): command is %s", cmd->name);

  // Special cases: '{', 'bind', 'unbound':
  //  These commands do not need argc/argv
  if (cmd == &command_bind || cmd == &command_group || cmd == &command_unbound) {
    cmdarg = cmd->parse(0, NULL, NULL, &cmd);
    goto ERROR_OR_END_WITHOUT_FREE;
  }

  // Is a normal command, build argc/argv:
  // Command arguments end in ';', '&&', '||', '\n', '}' (in case of block)
  for (;;) {
    ttype = lex_lex();
    debug("command_parse(): type is %s", LEX_TOK2STR(ttype));

    switch (ttype) {
      case LEX_TOKEN_WORD:
        if (argc == COMMAND_MAX_ARGS) {
          error_set(E2BIG, "Max length is " TO_STR(COMMAND_MAX_ARGS));
          goto ERROR_OR_END;
        }

        args[argc++] = strdup(lex_token());
        break;

      case EOF:
      case LEX_TOKEN_OR:
      case LEX_TOKEN_AND:
      case LEX_TOKEN_NEW_LINE:
      case LEX_TOKEN_BLOCK_END:
      case LEX_TOKEN_SEMICOLON:
        lex_unlex();
        goto BREAK_LOOP;

      default: debug("command_parse(): default?");
    }
  }

BREAK_LOOP:
  debugArray(args, argc);

  if (cmd->opts != NULL) {
    int  i = 0;
    char optstr[64];
    for (const command_opt_t *opt = cmd->opts; opt->opt; ++opt) {
      optstr[i++] = opt->opt;
      if (opt->meta)
        optstr[i++] = ':';
    }
    optstr[i] = 0;

    argc_old = argc;
    if (! get_options(&argc, &args, optstr, &options))
      goto ERROR_OR_END;
  }

  if (cmd->args == NULL) {
    if (argc > 0) {
      error_set_errno(E_SPARE_ARGS);
      goto ERROR_OR_END;
    }
  }
  else
    if (! check_args(argc, cmd->args))
      goto ERROR_OR_END;

  if (cmd->parse != NULL)
    cmdarg = cmd->parse(argc, args, options, &cmd);
  else
    cmdarg = (void*) 1;

ERROR_OR_END:
  free(options);

  if (argc_old)
    argc = argc_old;

  while (argc--)
    free(args_data[argc]);

ERROR_OR_END_WITHOUT_FREE:
  if (cmdarg) {
    if (! store)
      store = calloc(1, sizeof(command_call_t));
    store->arg     = cmdarg;
    store->command = cmd;
    return store;
  }
  else {
    error_add(cmd->name);
  }

  return NULL;
}

