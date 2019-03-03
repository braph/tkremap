#include "tkremap.h"

static COMMAND_CALL_FUNC(cmd_unbind) {
   char **args;
   unpackargs(NULL, &args, NULL, (command_args_t*) cmd->arg);

   TermKeyKey tkey;

   if (! parse_key(args[0], &tkey))
      return 0;

   binding_del_binding(context.current_mode->root, &tkey);
   return 1;
}

command_t command_unbind = {
   .name  = "unbind",
   .desc  = "Unbind key to commands",
   .args  = (const char*[]) { "+KEY", "+COMMAND", 0 },
   .parse = &copyargs,
   .free  = &deleteargs,
   .call  = &cmd_unbind
};

