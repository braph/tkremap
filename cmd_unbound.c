#include "tkremap.h"
#include "common.h"

static COMMAND_CALL_FUNC(cmd_unbound) {
   int argc;
   char **args;
   unpackargs(&argc, &args, NULL, (command_args_t*) cmd->arg);

   int action = UNBOUND_IGNORE;

   if (streq(args[argc - 1], "pass"))
      action = UNBOUND_PASS;
   else if (streq(args[argc - 1], "reeval"))
      action = UNBOUND_REEVAL;
   else if (streq(args[argc - 1], "ignore"))
      { }
   else {
      write_error("unknown action: %s", args[argc - 1]);
      return;//TODO 0;
   }

   for (int i = argc - 1; i--; )
      if (streq(args[i], "all")) {
         context.current_mode->unbound_unicode  = action;
         context.current_mode->unbound_keysym   = action;
         context.current_mode->unbound_function = action;
      }
      else if (streq(args[i], "char"))
         context.current_mode->unbound_unicode  = action;
      else if (streq(args[i], "sym"))
         context.current_mode->unbound_keysym   = action;
      else if (streq(args[i], "function"))
         context.current_mode->unbound_function = action;
      else if (streq(args[i], "mouse"))
         context.current_mode->unbound_mouse    = action;
      else {
         write_error("unknown key type: %s", args[i]);
         return;//TODO 0;
      }

   return;//TODO 1;
}

command_t command_unbound = {
   COMMAND_T_INIT,
   .name  = "unbound",
   .desc  =
      "What to do on unbound keys\n\n"
      "_TYPE_\n"
      " *char*     characters\n"
      " *sym*      symbolic keys or modified key\n"
      " *function* function keys (F1..FN)\n"
      " *mouse*    mouse events\n"
      " *all*      char|sym|function\n\n"
      "_ACTION_\n"
      " *ignore*   discard key\n"
      " *pass*     write key to program\n"
      " *reeval*   re-evaluate (leave key chain, rehandle key again)",
   .args  = (const char*[]) { "+TYPE", "ACTION", 0 },
   .parse = &copyargs,
   .free  = &deleteargs,
   .call  = &cmd_unbound,
};

