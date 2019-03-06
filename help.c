/* No memory management here, since help is a dead op */

#include "tkremap.h"
#include "commands.h"

#include <unistd.h>
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

#define EXAMPLE_TEXT \
   "tkremap can also be used for macros \n"\
   "tkremap -b 'Enter write \"set shiftwidth=2\n set tabstop=2\n gg=G:wg\n\""
   "readline -P exec -p 'sh $ '" \
   "bind ^C ^C ^C ^C ^C signal KILL" \

#define P(...) \
  printf(__VA_ARGS__)

#define PA(FMT, ...) \
  printf(fill_attrs(FMT), ##__VA_ARGS__)

#define ATTRS(...) \
  fill_attrs(__VA_ARGS__)

static char* firstline(const char *s) {
  return strndup(s, strcspn(s, "\n"));
}

static int has_more_lines(const char *s) {
  return !!strchr(s, '\n');
}

static const char* more_lines(const char *s) {
  return s += strcspn(s, "\n");
}

static void pad_right(int n) {
  while (n--)
    putchar(' ');
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
static void  print_all(const char *prog, const char *usage);

static char* fill_attrs(const char *);
static char* indent(const char *, int);

int help(const char *prog, const char *usage, const char *topic) {
  command_t *cmd = 0;

  if (! topic)
    PA(usage, prog);
  else if (streq(topic, "all"))
    print_all(prog, usage);
  else if (streq(topic, "keys"))
    help_keys();
  else if (streq("commands", topic))
    help_commands(0);
  else {
    if ((cmd = get_command(topic)))
      help_command(cmd, 1);
    else {
      PA(usage, prog);
      P("%s: %s", E_UNKNOWN_CMD, topic);
    }
  }

  return 0;
}

static void print_all(const char *prog, const char *usage) {
  PA(usage, prog);
  help_commands(1);
  help_keys();
}

static void help_commands(int full) {
  PA("_Commands_\n\n");
  for (int i = commands_size; i--; )
    help_command(commands[i], full);
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
      sprintf(r, "[-%s]", flags);
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
         PA(" _%s_...",    *arg + 1);
      else if (**arg == '*')
         PA(" [_%s_...]",  *arg + 1);
      else
         PA(" _%s_",       *arg);
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

      PA(firstline(opt->desc));
      if (has_more_lines(opt->desc))
        PA(indent(more_lines(opt->desc), 6 + max));
      P("\n");
    }
  }

  P("\n");
}

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
      case '*': // index = 42 % 2
      case '_': // index = 95 % 2
#ifndef README
        if (isatty(1))
#endif
          strcat(r, attrs[(2*(*s%2)) + (state = !state)]);
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
      "  Up/Down/Left/Right, PageUp/PageDown, Home/End, Insert/Delete,\n"
      "  Escape, Space, Enter, Tab, Backspace, F1 .. F12\n"
      "\n"
      " *Modifiers*\n"
      "  *Control*: Control-key, Ctrl-key, C-key, ^key\n"
      "  *Alt*:     Alt-key, A-key, Meta-key, M-key\n"
      "  *Shift*:   Shift-key, S-key\n"
      "\n"
      " *Special*\n"
      "\n"
    );
}

