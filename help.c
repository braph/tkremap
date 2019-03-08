/* No memory management here, since help is a dead op */

#include "tkremap.h"
#include "termkeystuff.h"
#include "commands.h"

#include <unistd.h>
#include <string.h>

#if README
#define BOLD        "**"
#define BOLD_END    "**"
#define ITALIC      "_"
#define ITALIC_END  "_"
#else
#define README 0
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
#endif // README

#if README
const char * const EXAMPLES = 
  "Reindent a buch of *.c files:\n"
  " tkremap -c 'bind Enter write \"set shiftwidth=2\n set tabstop=2\n gg=G:wg\n\"  vim *.c"
  //"readline -P exec -p 'sh '"

  "Configuration format:\n"
  " Configuration lines end with a newline or a semicolon (;).\n"
  " A line can be continued using the line continuation character (\\).\n"
  " Multiple commands can be specified using \\; or &&\n";
#endif

#define P(STR) \
  fputs(STR, stdout);

#define PF(...) \
  printf(__VA_ARGS__)

#define PFA(FMT, ...) \
  printf(fill_attrs(FMT), ##__VA_ARGS__)

#define PA(STR) \
  fputs(fill_attrs(STR), stdout)

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
    PFA(usage, prog);
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
      PFA(usage, prog);
      PF("%s: %s\n", E_UNKNOWN_CMD, topic);
    }
  }

  return 0;
}

static void print_all(const char *prog, const char *usage) {
  PFA(usage, prog);
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
    PFA("*%-15s*", cmd->name);
    PA(firstline(cmd->desc));
    P("\n");
    return;
  }

  PFA("*%s*", cmd->name);

  if (cmd->opts) {
    P(" ");
    PA(get_option_string(cmd));
  }

  if (cmd->args) {
    for (const char **arg = cmd->args; *arg; ++arg)
      if (**arg == '+')
         PFA(" _%s_...",    *arg + 1);
      else if (**arg == '*')
         PFA(" [_%s_...]",  *arg + 1);
      else
         PFA(" _%s_",       *arg);
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
      PFA(" *-%c*", opt->opt);

      if (opt->meta) {
        PFA(" _%s_", opt->meta);
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
  char *res = calloc(1, strlen(str) + 1024);

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

// Replace pseudo markdown (*_) with attributes
static char* fill_attrs(const char *s) {
  int state = 1;
  char *r = calloc(1, strlen(s) + 1024);

  do {
    switch (*s) {
      case '*': // index = 42 % 2
      case '_': // index = 95 % 2
        if (README || isatty(1))
          strcat(r, attrs[(2*(*s%2)) + (state = !state)]);
        break;
      case '\\':  strncat(r, ++s, 1);
                  break;
      default:    strncat(r, s, 1);
    }
  } while (*++s);

  return r;
}

#include <time.h>
static void help_keys() {
  PA(
      "_Keys_\n\n"
      " *Symbolic keys*\n"
      "  Up/Down/Left/Right, PageUp/PageDown, Home/End, Insert/Delete,\n"
      "  Escape, Space, Enter, Tab, Backspace, F1 .. F12\n"
      "\n"
      " *Modifiers*\n"
      "  *Control*: Ctrl-key, C-key, ^key\n"
      "  *Alt*:     Alt-key, A-key, Meta-key, M-key\n"
      "  *Shift*:   Shift-key, S-key\n"
      "\n"
    );

  char buf[99];
  TermKeyKey key;
  srand(time(NULL));
  int t, c, m, f;
  for (int i = 0; i < 20; ++i) {
    t = rand(), c = rand(), m = rand(), f = rand();

    #define case break; case
    switch ((int) (key.type = (t % 3))) {
      case TERMKEY_TYPE_UNICODE:
        key.code.codepoint = (c % 'Z') + 'a';
      case TERMKEY_TYPE_KEYSYM:
        key.code.sym       = (c % TERMKEY_SYM_END) + 1;
      case TERMKEY_TYPE_FUNCTION:
        key.code.number    = (c % 12) + 1;
    }
    #undef case

    key.modifiers = (m & TERMKEY_KEYMOD_ALT) | (m & TERMKEY_KEYMOD_CTRL) | (m & TERMKEY_KEYMOD_SHIFT);
    f = (f & TERMKEY_FORMAT_LONGMOD) | (f & TERMKEY_FORMAT_CARETCTRL) | (f & TERMKEY_FORMAT_ALTISMETA);
    termkey_strfkey(tk, buf, 99, &key, f);
    PFA(" *%s*,", buf);
  }
}

