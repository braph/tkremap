#include "common.h"
#include "errormsg.h"
#include "commands.h"

extern command_t command_write;
extern command_t command_unbound;
extern command_t command_unbind;
extern command_t command_signal;
extern command_t command_readline;
extern command_t command_rehandle;
extern command_t command_repeat;
extern command_t command_pass;
extern command_t command_mode;
extern command_t command_mask;
extern command_t command_load;
extern command_t command_key;
extern command_t command_ignore;
extern command_t command_bind;

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
   &command_bind
};
int commands_size = (sizeof(commands)/sizeof(commands[0]));

command_t* get_command(const char *name) {
   command_t *cmd = NULL;

   for (int i = commands_size; i--; )
      if (strprefix(commands[i]->name, name)) {
         if (cmd)
            return write_error("%s: %s", E_AMBIGIOUS_CMD, name), NULL;
         else
            cmd = commands[i];
      }

   if (! cmd)
      write_error("%s: %s", E_UNKNOWN_CMD, name);

   return cmd;
}

void* command_parse(command_t *cmd, int argc, char **args) {
   void   *ret     = NULL;
   option *options = NULL;

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
         write_error("spare arguments");
         goto ERROR_OR_END;
      }
   }
   else
      if (! check_args(argc, cmd->args))
         goto ERROR_OR_END;

   if (cmd->parse != NULL)
      ret = cmd->parse(argc, args, options);
   else
      ret = (void*) 1;

ERROR_OR_END:
   free(options);
   return ret; // is NULL if failed
}

