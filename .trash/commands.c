
command_t* get_command(const char *name) {
  command_t *cmd = NULL;

  for (int i = commands_size; i--; )
    if (strprefix(commands[i]->name, name)) {
      if (cmd)
        return error_set_errno(E_AMBIGIOUS_CMD), NULL;
      else
        cmd = commands[i];
    }

  if (! cmd)
    error_set_errno(E_UNKNOWN_CMD);

  return cmd;
}

// commands.h: extern const int  COMMANDS_SIZE;
const int COMMANDS_SIZE = (sizeof(commands) / sizeof(commands[0]));
