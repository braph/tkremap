#include "conf.h"
#include "lexer.h"
#include "common.h"
#include "tkremap.h"
#include "commands.h"
#include "errormsg.h"
#include "termkeystuff.h"

#include <string.h>

static int lex_args(char ***args) {
   int ttype;
   int n = 0;
   *args = NULL;

   if (lex_eof())
      return EOF;

   while ((ttype = lex()) != EOF) {
      if (ttype == LEX_ERROR)
         return LEX_ERROR;

      if (ttype == LEX_TOKEN_END)
         break;

      *args = realloc(*args, ++n * sizeof(char*));
      (*args)[n - 1] = strdup(lex_token());
   }

   return n;
}

int read_conf_stream(FILE *fh) {
   int         ret = 1;
   char        **args = NULL;
   int         nargs;

   void           *cmdarg;
   command_t      *command;
   keymode_t      *mode_restore = context.current_mode;
   command_call_t cmdcall;

   lex_init(fh);

   while ((nargs = lex_args(&args)) != EOF) {
      if (nargs == LEX_ERROR) {
         error_write("%s", lex_error());
         ret = 0;
         goto END;
      }

      if (nargs == 0)
         continue;

      if (! (command = get_command(args[0]))) {
         error_add("%d:%d", lex_line, lex_line_pos);
         ret = 0;
         goto END;
      }

      if (! (cmdarg = command_parse(command, nargs - 1, &args[1]))) {
         error_add("%d:%d: %s", lex_line, lex_line_pos, command->name);
         ret = 0;
         goto END;
      }

      cmdcall.arg     = cmdarg;
      if (! command->call(&cmdcall, NULL)) {
         if (command->free)
            command->free(cmdarg);
         ret = 0;
         goto END;
      }
      if (command->free)
         command->free(cmdarg);

      freeArray(args, nargs);
      args = NULL;
   }

END:
   if (args)
      freeArray(args, nargs);
   lex_destroy();
   context.current_mode = mode_restore;
   return ret;
}

int read_conf_string(const char *str) {
   FILE *fh = fmemopen((void*) str, strlen(str), "r");

   if (! fh)
      return error_write("%s", strerror(errno)), 0;

   int ret = read_conf_stream(fh);
   fclose(fh);
   return ret;
}

int read_conf_file(const char *file) {
   FILE *fh = fopen(file, "r");

   if (! fh)
      return error_write("%s", strerror(errno)), 0;

   int ret = read_conf_stream(fh);
   fclose(fh);
   return ret;
}
