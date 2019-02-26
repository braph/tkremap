#include "conf.h"
#include "lexer.h"
#include "common.h"
#include "tkremap.h"
#include "commands.h"
#include "errormsg.h"
#include "termkeystuff.h"

#include <string.h>

// bind_parse.c
int binding_append_commands(binding_t*, int, char *[]);

// cmd_load.c
int load_conf(const char *);

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

// === mode ===================================================================
static int mode(int argc, char *args[], option *options) {
   if (! (context.current_mode = get_keymode(args[0])))
      context.current_mode = add_keymode(args[0]);
   return 1;
}

conf_command_t conf_mode = {
   .name  = "mode",
   .desc  = "Start configuring MODE",
   .args  = (const char*[]) { "MODE", 0 },
   .opts  = NULL,
   .parse = &mode
};
// ============================================================================

// === unbound ========================================================
static int unbound(int argc, char *args[], option *options) {
   int action = UNBOUND_IGNORE;

   if (streq(args[argc - 1], "pass"))
      action = UNBOUND_PASS;
   else if (streq(args[argc - 1], "reeval"))
      action = UNBOUND_REEVAL;
   else if (streq(args[argc - 1], "ignore"))
      { }
   else {
      write_error("unknown action: %s", args[argc - 1]);
      return 0;
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
         return 0;
      }

   return 1;
}

conf_command_t conf_unbound = {
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
   .opts  = NULL,
   .parse = &unbound
};
// ============================================================================

// === repeat =================================================================
static int repeat(int argc, char *args[], option *options) {
   if (streq(args[0], "on"))
      context.current_mode->repeat_enabled = 1;
   else if (streq(args[0], "off"))
      context.current_mode->repeat_enabled = 0;
   else {
      write_error("%s: '%s' {on|off}", strerror(EINVAL), args[0]);
      return 0;
   }

   return 1;
}

conf_command_t conf_repeat = {
   .name  = "repeat",
   .desc  = "Enable repetition mode",
   .args  = (const char*[]) { "on|off", 0 },
   .opts  = NULL,
   .parse = &repeat
};
// ============================================================================

// === bind ===================================================================
static int bind(int argc, char *args[], option *options) {
   TermKeyKey key;
   binding_t  *binding, *binding_next;

   binding = context.current_mode->root;

   // read till last keydef
   while (argc > 1 && parse_key(args[0], &key)) {
      args_get_arg(&argc, &args, NULL);
      binding_next = binding_get_binding(binding, &key);

      if (! binding_next) {
         binding_next             = malloc(sizeof(binding_t));
         binding_next->key        = key;
         binding_next->size       = 0;
         binding_next->p.commands = NULL;
         binding_next->type       = BINDING_TYPE_CHAINED;
         binding = binding_add_binding(binding, binding_next);
      }
      else {
         if (binding_next->type == BINDING_TYPE_COMMAND) {
            write_error("Overwriting key binding"); // TODO
            return 0;
         }

         binding = binding_next;
      }
   }

   if (binding == context.current_mode->root) {
      write_error("%s: %s", E_MISSING_ARG, "KEY");
      return 0;
   }

   if (binding->type == BINDING_TYPE_COMMAND) {
      write_error("Overwriting key binding");
      return 0;
   }

   if (binding->type == BINDING_TYPE_CHAINED && binding->size > 0) {
      write_error("Overwriting key binding");
      return 0;
   }

   if (argc == 0) {
      write_error("%s: %s", E_MISSING_ARG, "COMMAND");
      return 0;
   }

   binding->type = BINDING_TYPE_COMMAND;
   return binding_append_commands(binding, argc, args);
}

conf_command_t conf_bind = {
   .name  = "bind",
   .desc  = 
      "Bind _KEY_ to _COMMAND_\n"
      "Multiple commands can be specified, they have to be seperated by '\\\\;'.\n"
      "Keys can be chained",
   .args  = (const char*[]) { "+KEY", "+COMMAND", 0 },
   .opts  = NULL,
   .parse = &bind
};
// ============================================================================

// === unbind =================================================================
static int unbind(int argc, char *args[], option *options) {
   TermKeyKey key;

   if (! parse_key(args[0], &key))
      return 0;

   binding_del_binding(context.current_mode->root, &key);
   return 1;
}

conf_command_t conf_unbind = {
   .name  = "unbind",
   .desc  = "Unbind key to commands",
   .args  = (const char*[]) { "+KEY", "+COMMAND", 0 },
   .opts  = NULL,
   .parse = &unbind
};
// ============================================================================

// === load ===================================================================
static int load(int argc, char *args[], option *options) {
   return load_conf(args[0]);
}

conf_command_t conf_load = {
   .name  = "load",
   .desc  = "Load configuration file\nSee command load",
   .args  = (const char*[]) { "FILE", 0 },
   .opts  = NULL,
   .parse = &load
};
// ============================================================================

conf_command_t *conf_commands[] = {
   &conf_repeat,
   &conf_load,
   &conf_unbound,
   &conf_unbind,
   &conf_bind,
   &conf_mode
};

int conf_commands_size = (sizeof(conf_commands)/sizeof(conf_commands[0]));

conf_command_t* get_conf_command(const char *name) {
   return (conf_command_t*) _get_command(
      (command_t**) conf_commands, conf_commands_size, name);
}

int read_conf_stream(FILE *fh) {
   int         ret = 1;
   char        **args = NULL;
   int         nargs;

   conf_command_t *conf_command;
   keymode_t      *mode_restore = context.current_mode;

   lex_init(fh);

   while ((nargs = lex_args(&args)) != EOF) {
      if (nargs == LEX_ERROR) {
         write_error("%s", lex_error());
         ret = 0;
         goto END;
      }

      if (nargs == 0)
         continue;

      if (! (conf_command = get_conf_command(args[0]))) {
         prepend_error("%d:%d", lex_line, lex_line_pos);
         ret = 0;
         goto END;
      }
      else if (! command_parse((command_t*) conf_command, nargs - 1, &args[1])) {
         prepend_error("%d:%d: %s", lex_line, lex_line_pos, conf_command->name);
         ret = 0;
         goto END;
      }

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

   if (! fh) {
      write_error("%s", strerror(errno));
      return 0;
   }

   int ret = read_conf_stream(fh);
   fclose(fh);
   return ret;
}

int read_conf_file(const char *file) {
   FILE *fh = fopen(file, "r");

   if (! fh) {
      write_error("%s", strerror(errno));
      return 0;
   }

   int ret = read_conf_stream(fh);
   fclose(fh);
   return ret;
}
