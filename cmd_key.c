#include "tkremap.h"

#include <string.h>
#include <stdint.h>
#include <unistd.h>

typedef struct cmd_key_args {
   uint16_t repeat;
   char     string[1];
} cmd_key_args;

static COMMAND_CALL_FUNC(call) {
   cmd_key_args *arg = (cmd_key_args*) cmd->arg;

   for (int i = arg->repeat; i--; )
      writes_to_program(arg->string);
}

static COMMAND_PARSE_FUNC(parse) {
   const char   *seq;
   int           repeat    = 1;
   cmd_key_args *cmd_args  = NULL;
   char         *sequences = malloc(argc * 16);
   sequences[0] = 0;

   for (option *opt = options; opt->opt; ++opt) {
      if (opt->opt == 'r')
         if ((repeat = atoi(opt->arg)) <= 0) {
            write_error("%s: %s", strerror(EINVAL), opt->arg);
            goto END_OR_ERROR;
         }
   }

   for (int i = 0; i < argc; ++i) {
      if (! (seq = key_parse_get_code(args[i])))
         goto END_OR_ERROR;
      strcat(sequences, seq);
   }

   cmd_args = malloc(sizeof(*cmd_args) + strlen(sequences));
   cmd_args->repeat = repeat;
   strcpy(cmd_args->string, sequences);

END_OR_ERROR:
   free(sequences);
   return (void*) cmd_args; // is NULL if failed
}

const command_t command_key = {
   .name  = "key",
   .desc  = "Send key to program",
   .args  = (const char*[]) { "+KEY", 0 },
   .opts  = (const command_opt_t[]) {
      {'r', "N", "Repeat the key N times"},
      {0,0,0}
   },
   .parse = &parse,
   .call  = &call,
   .free  = &free
};

