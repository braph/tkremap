#include "tkremap.h"
#include "commands.h"
#include "lexer.h"

#define CMD_SEPARATOR_SEMICOLON ((void*) 1)
#define CMD_SEPARATOR_AND       ((void*) 2)
#define CMD_SEPARATOR_OR        ((void*) 3)
#define CMD_GROUP_MAX_COMMANDS  1024

typedef struct cmd_group_args_t {
  int             size;
  command_call_t *commands[0];
} cmd_group_args_t;

static COMMAND_PARSE_FUNC(parse) {
  ___("parse");
  int ttype;
  int size = 0;
  command_call_t *commands[CMD_GROUP_MAX_COMMANDS];

  while (1) {
    ttype = lex_lex();

    // Skip newlines
    if (ttype == LEX_TOKEN_NEW_LINE)
      continue;

    // Put back the token for `command_parse`
    lex_unlex();

    // Parse command
    if (! (commands[size++] = command_parse(NULL, NULL))) {
      --size;

      // Could not parse command because `command_parse` tried '}'
      lex_unlex();
      if (lex_lex() == LEX_TOKEN_BLOCK_END)
        break;

      // Free commands that have been parsed
      for (int i = 0; i < size; i += 2) {
        command_call_free(commands[i]);
        free(commands[i]);
      }

      return NULL;
    }

    // Get separator
    ttype = lex_lex();
    debug("parse(): separator, parsed: %s", LEX_TOK2STR(ttype));
    switch (ttype) {
      case LEX_TOKEN_NEW_LINE:
      case LEX_TOKEN_SEMICOLON: commands[size++] = CMD_SEPARATOR_SEMICOLON; break;
      case LEX_TOKEN_AND:       commands[size++] = CMD_SEPARATOR_AND;       break;
      case LEX_TOKEN_OR:        commands[size++] = CMD_SEPARATOR_OR;        break;
      case LEX_TOKEN_BLOCK_END: goto BREAK_LOOP;
      //case EOF: error! TODO
    }
  }

BREAK_LOOP:
  debug("parse(): size = %d", size);

  if (size == 0) {
    error_set(E_MISSING_ARG, "COMMAND");
    return NULL;
  }

  if (size == 1) {
    *switch_command = commands[0]->command;
    return commands[0]->arg;
  }

  #define SIZE (size * sizeof(void*))
  cmd_group_args_t *cmd_args = malloc(sizeof(cmd_group_args_t) + SIZE);
  cmd_args->size = size;
  memcpy(cmd_args->commands, commands, SIZE);
  return cmd_args;
}

static COMMAND_CALL_FUNC(call) {
  ___("call");
  cmd_group_args_t *cmd_args = (cmd_group_args_t*) cmd->arg;
  int last_return;

  for (int i=0; i < cmd_args->size; ++i) {
    assert(! (i % 2));
    last_return = command_call_execute(cmd_args->commands[i], key);

    // stop execution if command separators is && and command failed
CHECK_CMD_SEPARATOR:
    if (++i < cmd_args->size) {
      if (cmd_args->commands[i] == CMD_SEPARATOR_AND && !last_return) {
        ++i; // "next command"
        goto CHECK_CMD_SEPARATOR;
      }

      if (cmd_args->commands[i] == CMD_SEPARATOR_OR && last_return) {
        ++i; // "next command"
        goto CHECK_CMD_SEPARATOR;
      }

      // CMD_SEPARATOR_SEMICOLON
    }
  }

  return 1; // TODO: adjust return value
}

static void cmd_group_free(void *args) {
  cmd_group_args_t *cmd_args = (cmd_group_args_t*) args;

  for (int i=0; i < cmd_args->size; ++i) {
    command_call_free(cmd_args->commands[i]);
    free(cmd_args->commands[i]);
    ++i; // skip command separator (';', '&&', '||')
  }

  free(args);
}

const command_t command_group = {
  .name  = "{"
    "\0Group multiple commands\n"
    //"Use this to group multiple commands together.\n"
    "Available command separators: '*;*', '*&&*', '*||*', '*\\\\n*'",
  .args  = "+COMMAND\0}\0",
  .call  = &call,
  .parse = &parse,
  .free  = &cmd_group_free
};

