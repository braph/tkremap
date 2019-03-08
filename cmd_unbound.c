#include "tkremap.h"
#include "commands.h"

#define UNBOUND_UNICODE  (1 << TERMKEY_TYPE_UNICODE)
#define UNBOUND_KEYSYM   (1 << TERMKEY_TYPE_KEYSYM)
#define UNBOUND_FUNCTION (1 << TERMKEY_TYPE_FUNCTION)
#define UNBOUND_MOUSE    (1 << TERMKEY_TYPE_MOUSE)
#define UNBOUND_ANY      (UNBOUND_UNICODE|UNBOUND_KEYSYM|UNBOUND_FUNCTION)

static COMMAND_CALL_FUNC(cmd_unbound) {
  int argc;
  char **args;
  unpackargs(&argc, &args, NULL, (command_args_t*) cmd->arg);

  // Read TYPEs
  int i, flags = 0;
  for (i = 0; i < argc; ++i)
    if (streq(args[i], "any"))
      flags |= UNBOUND_ANY;
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
    flags = UNBOUND_ANY;

  // Build commands
  commands_t *commands[4] = { 0, 0, 0, 0 };

  for (int f = 0; f < 4; ++f) {
    if (flags & (1 << f)) {
      if (! (commands[f] = commands_parse(argc - i, &args[i]))) {
        while (--f >= 0)
          if (commands[f]) {
            commands_free(commands[f]);
            free(commands[f]);
          }
        return 0;
      }
    }
  }

  // Replace commands
  keymode_t *km = context.current_mode;

  for (int f = 0; f < 4; ++f) {
    if (commands[f]) {
      if (km->unbound[f]) {
        commands_free(km->unbound[f]);
        free(km->unbound[f]);
      }

      km->unbound[f] = commands[f];
    }
  }

  return 1;
}

command_t command_unbound = {
  .name  = "unbound",
  .desc  =
    "Specify action for unbound keys\n\n"
    "_TYPE_\n"
    " *char*     characters\n"
    " *sym*      symbolic keys or modified key\n"
    " *function* function keys\n"
    " *mouse*    mouse events\n"
    " *any*      char|sym|function [*default*]\n"
    "\n"
    "_COMMAND_\n"
    " Most of the time you want to use the following commands:\n\n"
    " *pass*     - for passing the key as is to the program\n"
    " *ignore*   - for completely ignoring the key\n"
    " *rehandle* - for rehandling the key (in conjunction with preceding *mode* command)",
  .args  = (const char*[]) { "*TYPE", "+COMMAND", 0 },
  .parse = &copyargs,
  .free  = &deleteargs,
  .call  = &cmd_unbound
};

