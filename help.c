/* No memory management here, since help is a dead op */

#include "conf.h"
#include "common.h"
#include "unistd.h"
#include "errormsg.h"
#include "commands.h"

#include <string.h>

#ifndef BOLD
#define BOLD        "\033[1m"
#endif
#ifndef BOLD_END
#define BOLD_END    "\033[0m"
#endif
#ifndef ITALIC
#define ITALIC      "\033[4m"
#endif
#ifndef ITALIC_END
#define ITALIC_END  "\033[0m"
#endif

#define P(...) \
   printf(__VA_ARGS__)

#define PA(FMT, ...) \
   printf(fill_attrs(FMT), ##__VA_ARGS__)

#define ATTRS(...) \
   fill_attrs(__VA_ARGS__)

static char* firstline(const char *s) {
   return strndup(s, strcspn(s, "\n"));
}

static void pad_right(int n) {
   while (n--)
      P(" ");
}

static const char *attrs[] = {
   BOLD,
   BOLD_END,
   ITALIC,
   ITALIC_END
};

static void  help_keys();
static void  help_command(command_t *cmd, int full);
static void  help_commands(int full);
static void  help_conf_commands(int full);
static void  print_all(const char *prog, const char *usage);
static void  print_usage(const char *prog, const char *usage);

static char* fill_attrs(const char *);
static char* indent(const char *, int);

int help(const char *prog, const char *usage, const char *topic) {
   command_t *cmd = 0, *conf = 0;

   if (! topic)
      print_usage(prog, usage);
   else if (streq(topic, "all"))
      print_all(prog, usage);
   else if (streq(topic, "keys"))
      help_keys();
   else if (strprefix("commands", topic))
      help_commands(0);
   else if (strprefix("config", topic))
      help_conf_commands(0);
   else {
      cmd  = get_command(topic);
      conf = (command_t*) get_conf_command(topic);

      if (cmd) {
         if (conf) P("(Command)\n");
         help_command(cmd, 1);
      }
      if (conf) {
         if (cmd)  P("(Config)\n");
         help_command(conf, 1);
      }
      if (!cmd && !conf) {
         print_usage(usage, prog);
         P("%s: %s", E_UNKNOWN_CMD, topic);
      }
   }

   return 0;
}

static void print_usage(const char *prog, const char *usage) {
   PA(usage, prog);
   P("\n");
}

static void print_all(const char *prog, const char *usage) {
   print_usage(prog, usage);
   help_conf_commands(1);
   help_commands(1);
   help_keys();
}

static void help_commands(int full) {
   PA("_Available commands_\n\n");
   for (int i = commands_size; i--; )
      help_command(commands[i], full);
   P("\n");
}

static void help_conf_commands(int full) {
   PA("_Configuration keywords_\n\n");
   for (int i = conf_commands_size; i--; )
      help_command((command_t*) conf_commands[i], full);
   P("\n");
}

// Returns [-fb] [-a arg] [-s arg]
static char* get_option_string(command_t *cmd ) {
   char opts[128],  *o = opts;
   char flags[128], *f = flags;
   char *r = calloc(1, 1024);

   if (cmd->opts) {
      for (const command_opt_t *opt = cmd->opts; opt->opt; ++opt)
         if (opt->meta)
            o += sprintf(o, " [-%c %s]", opt->opt, opt->meta);
         else
            *f++ = opt->opt;
      *f = *o = 0;

      if (strlen(flags))
         strcat(r, "[-"), strcat(r, flags), strcat(r, "]");
      if (strlen(opts))
         strcat(r, opts + !strlen(flags));
   }

   return r;
}

static void help_command(command_t *cmd, int full) {
   if (! full) {
      PA("*%-15s*", cmd->name);
      PA(firstline(cmd->desc));
      PA("\n");
      return;
   }

   PA("*%s*", cmd->name);

   if (cmd->opts) {
      P(" ");
      PA(get_option_string(cmd));
   }

   if (cmd->args) {
      for (const char **arg = cmd->args; *arg; ++arg)
         if (**arg == '+')
            PA(" _%s_...", *arg + 1);
         else
            PA(" _%s_", *arg);
   }

   P("\n");
   P(indent(ATTRS(cmd->desc), 1));
   P("\n");

   if (cmd->opts) {
      P("\n");

      int max = 0;
      for (const command_opt_t *opt = cmd->opts; opt->opt; ++opt)
         if (opt->meta)
            max = (strlen(opt->meta) > max ? strlen(opt->meta) : max);

      for (const command_opt_t *opt = cmd->opts; opt->opt; ++opt) {
         PA(" *-%c*", opt->opt);

         if (opt->meta) {
            PA(" _%s_", opt->meta);
            pad_right(3 + max - strlen(opt->meta) - 1);
         }
         else
            pad_right(3 + max);

         PA("%s\n", opt->desc);
      }
   }

   P("\n");
}

/*
static void help_command(command_t *cmd, int full) {
   if (! full) {
      PA("*%-15s* ", cmd->name);
      PA(firstline(cmd->desc));
      PA("\n");
      return;
   }

   PA("*%s*", cmd->name);

   if (cmd->opts)
      P(" [OPTIONS]");

   if (cmd->args) {
      for (const char **arg = cmd->args; *arg; ++arg)
         if (**arg == '+')
            PA(" _%s_ ...", *arg + 1);
         else
            PA(" _%s_ ",    *arg);
   }

   P("\n");
   P(indent(ATTRS(cmd->desc), 1));
   P("\n");

   if (cmd->opts) {
      P("\n");

      int max = 0;
      for (const command_opt_t *opt = cmd->opts; opt->opt; ++opt)
         if (opt->meta)
            max = (strlen(opt->meta) > max ? strlen(opt->meta) : max);

      for (const command_opt_t *opt = cmd->opts; opt->opt; ++opt) {
         PA(" *-%c*", opt->opt);

         if (opt->meta) {
            PA(" _%s_", opt->meta);
            pad_right(3 + max - strlen(opt->meta) - 1);
         }
         else
            pad_right(3 + max);

         PA("%s\n", opt->desc);
      }
   }

   P("\n");
}
*/

static
char * indent(const char *str, int pad) {
   int   ind = -1;
   char *res = calloc(1, strlen(str) * 2);

   for (int i = pad; i--; )
      res[++ind] =  ' ';

   do {
      res[++ind] = *str;
      if (*str == '\n')
         for (int i = pad; i--; )
            res[++ind] = ' ';
   } while (*++str);

   return res;
}

// Replace markdown (*_) with attributes
static char* fill_attrs(const char *s) {
   int state = 1;
   char *r = calloc(1, strlen(s) * 2);

   do {
      switch (*s) {
         case '*':   
            if (isatty(1))
               strcat(r, attrs[0 + (state = !state)]);
            break;
         case '_':
            if (isatty(1))
               strcat(r, attrs[2 + (state = !state)]);
            break;
         case '\\':  strncat(r, ++s, 1);
            break;
         default:    strncat(r, s, 1);
      }
   } while (*++s);

   return r;
}

static void help_keys() {
   PA(
      "_Keys_\n\n"
      " *Symbolic keys*\n"
      "  Up/Down/Left/Right, PageUp/PageDown, Home/End,\n"
      "  Insert/Delete, Space, Enter, Tab, Backspace, F1 .. F12\n"
      "\n"
      " *Modifiers*\n"
      "  *Control*: Control-key, Ctrl-key, C-key, ^key\n"
      "  *Alt*:     Alt-key, A-key, Meta-key, M-key\n"
      "  *Shift*:   Shift-key, S-key\n"
      "\n"
   );
}

static void help_modes() {
   // TODO
}
