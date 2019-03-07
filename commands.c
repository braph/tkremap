#include "common.h"
#include "errormsg.h"
#include "commands.h"

extern command_t
  command_write,
  command_unbound,
  command_unbind,
  command_signal,
  command_readline,
  command_rehandle,
  command_repeat,
  command_pass,
  command_mode,
  command_mask,
  command_load,
  command_key,
  command_ignore,
  command_exec,
  command_command,
  command_bind;

command_t* commands[] = {
  &command_write,
  &command_unbound,
  &command_unbind,
  &command_signal,
  &command_repeat,
  &command_rehandle,
  &command_readline,
  &command_pass,
  &command_mode,
  &command_mask,
  &command_load,
  &command_key,
  &command_ignore,
  &command_exec,
  &command_command,
  &command_bind
};
int commands_size = (sizeof(commands)/sizeof(commands[0]));

command_t* get_command(const char *name) {
  command_t *cmd = NULL;

  for (int i = commands_size; i--; )
    if (strprefix(commands[i]->name, name)) {
      if (cmd)
        return error_write("%s: %s", E_AMBIGIOUS_CMD, name), NULL;
      else
        cmd = commands[i];
    }

  if (! cmd)
    error_write("%s: %s", E_UNKNOWN_CMD, name);

  return cmd;
}

command_call_t* command_parse(int argc, char **args, command_call_t *store) {
  void   *arg = NULL;
  option *options = NULL;
  char   *name = args_get_arg(&argc, &args, NULL);

  command_t *cmd = get_command(name);
  if (! cmd)
    return NULL;

  if (cmd->opts != NULL) {
    int  i = 0;
    char optstr[64];
    for (const command_opt_t *opt = cmd->opts; opt->opt; ++opt) {
      optstr[i++] = opt->opt;
      if (opt->meta)
        optstr[i++] = ':';
    }
    optstr[i] = 0;

    if (! get_options(&argc, &args, optstr, &options))
      return NULL;
  }

  if (cmd->args == NULL) {
    if (argc > 0) {
      error_write("spare arguments");
      goto ERROR_OR_END;
    }
  }
  else
    if (! check_args(argc, cmd->args))
      goto ERROR_OR_END;

  if (cmd->parse != NULL)
    arg = cmd->parse(argc, args, options);
  else
    arg = (void*) 1;

ERROR_OR_END:
  free(options);

  if (arg) {
    if (! store)
      store = malloc(sizeof(command_call_t));
    store->arg = arg;
    store->command =cmd;
    return store;
  }

  return NULL;
}

#define EQ_ANDAND(S) (S[0] ==  '&' && S[1] == '&' && S[2] == 0)
#define EQ_SEMICO(S) (S[0] == '\\' && S[1] == ';' && S[2] == 0)

// TODO: free if failed
/* parse multiple commands, append to commands */
#define COMMANDS_MAX 1024
commands_t* commands_parse(int argc, char *args[])
{
  if (! argc)
    return NULL;

  struct {
    uint32_t offset : 18;
    uint32_t length : 12;
    uint32_t op     :  2;
  } offsets[COMMANDS_MAX];

  unsigned i;
  unsigned offset_i = 0;
  commands_t     *commands;

  offsets[0].offset = 0;
  offsets[0].length = 0;
  for (i = 0; i < argc; ++i) {
    int op = -1;
    if (streq(args[i], "\\;"))
      op = COMMAND_SEPARATOR_SEMICOLON;
    else if (streq(args[i], "&&"))
      op = COMMAND_SEPARATOR_AND;
    else
      offsets[offset_i].length++;

    if (op >= 0) {
      offsets[offset_i].op = op;
      if (offsets[offset_i].length == 0)
        return NULL;
      ++offset_i;
      offsets[offset_i].offset = ++i;
      offsets[offset_i].length = 1;
    }
  }
  offsets[offset_i].op = COMMAND_SEPARATOR_SEMICOLON;

  commands = malloc(sizeof(commands_t));
  commands->size = (1 + offset_i) * 2 - 1;
  commands->commands = calloc((1 + offset_i) * 2 - 1, sizeof(command_call_t));

  for (i = -1; ++i <= offset_i;) {
    if (! (command_parse(offsets[i].length, &args[offsets[i].offset], &commands->commands[i*2])))
      return 0; // TODO

    if (i < offset_i)
      commands->commands[i*2+1].command = (command_t*) (uintptr_t) offsets[i].op;
  }

  return commands;
}
