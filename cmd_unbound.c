#include "tkremap.h"

// bind_parse.c
commands_t* commands_parse(int, char *[]);

#define UNBOUND_UNICODE  (1 << TERMKEY_TYPE_UNICODE)
#define UNBOUND_KEYSYM   (1 << TERMKEY_TYPE_KEYSYM)
#define UNBOUND_FUNCTION (1 << TERMKEY_TYPE_FUNCTION)
#define UNBOUND_MOUSE    (1 << TERMKEY_TYPE_MOUSE)

static COMMAND_CALL_FUNC(cmd_unbound) {
  int argc;
  char **args;
  unpackargs(&argc, &args, NULL, (command_args_t*) cmd->arg);

  int i, flags = 0;
  for (i = 0; i < argc; ++i)
    if (streq(args[i], "all"))
      flags |= (UNBOUND_UNICODE|UNBOUND_KEYSYM|UNBOUND_FUNCTION);
    else if (streq(args[i], "char"))
      flags |= UNBOUND_UNICODE;
    else if (streq(args[i], "sym"))
      flags |= UNBOUND_KEYSYM;
    else if (streq(args[i], "function"))
      flags |= UNBOUND_FUNCTION;
    else if (streq(args[i], "mouse"))
      flags |= UNBOUND_MOUSE;
    else
      break;

  if (flags == 0)
    flags = (UNBOUND_UNICODE|UNBOUND_KEYSYM|UNBOUND_FUNCTION);

  keymode_t *km = context.current_mode;

  for (int j = 0; j <= 3; ++j) {
    if (flags & (1 << j)) {
      if (km->unbound[j])
        commands_free(km->unbound[j]);
      else {
        km->unbound[j] = commands_parse(argc - i, &args[i]); // TODO check for NULL
        //error_write("unknown key type/command: %s", args[i]);
        return 1;
      }
    }
  }

  return 1;
}

command_t command_unbound = {
  COMMAND_T_INIT,
  .name  = "unbound",
  .desc  =
    "Specify action for unbound keys\n\n"
    "_TYPE_\n"
    " *char*     characters\n"
    " *sym*      symbolic keys or modified key\n"
    " *function* function keys\n"
    " *mouse*    mouse events\n"
    " *all*      char|sym|function [*default*]\n"
    "\n"
    "_COMMAND_\n"
    " Most of the time you want to use the following commands:\n\n"
    " *pass*     - for passing the key as is to the program\n"
    " *ignore*   - for completely ignoring the key\n"
    " *rehandle* - for rehandling the key (in conjunction with preceding *mode* command)"
    ,.args  = (const char*[]) { "*TYPE", "+COMMAND", 0 },
  .parse = &copyargs,
  .free  = &deleteargs,
  .call  = &cmd_unbound,
};

