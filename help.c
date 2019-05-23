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
static const char *attrs[] = { BOLD, BOLD_END, ITALIC, ITALIC_END };

#define PC(C) \
  printf("%c", C)

#define PA(STR) \
  printf("%s", fill_attrs(STR))

#define PFA(FMT, ...) \
  printf(fill_attrs(FMT), ##__VA_ARGS__)

#define sprintf(BUF, ...) \
  snprintf(BUF, 0xFFFF, __VA_ARGS__)

#define GET_CMD_DESCRIPTION(CMD_NAME) \
  (CMD_NAME + strlen(CMD_NAME) + 1)

static void __inline pad_right(int n) {
  printf("%*s", n, "");
  //while (n--) PC(' ');
}

static char* firstline(const char *, const char **);
static char* indent(const char *, int);
static char* fill_attrs(const char *);
static void  help_command(const command_t *cmd, int full);

#define HELP_USAGE          0x01
#define HELP_COMMANDS_SHORT 0x02
#define HELP_COMMANDS_FULL  0x04
#define HELP_KEYS           0x08
int help(const char *prog, const char *usage, const char *topic) {
  int flags = 0;
  const command_t *cmd = (command_t*) -1;

  if      (topic == NULL)            flags = HELP_USAGE;
  else if (streq(topic, "all"))      flags = HELP_USAGE|HELP_COMMANDS_FULL|HELP_KEYS;
  else if (streq(topic, "keys"))     flags = HELP_KEYS;
  else if (streq(topic, "commands")) flags = HELP_COMMANDS_SHORT;
  else if ((cmd = get_command(topic))) {
    PC('\n');
    help_command(cmd, 1);
  } else {
    flags = HELP_USAGE;
  }

  if (flags & HELP_USAGE)
    PFA(usage, prog);

  if (cmd == 0) {
    error_add(topic);
    printf("%s\n", error_get());
  }

  if (flags & HELP_COMMANDS_SHORT || flags & HELP_COMMANDS_FULL) {
    PA("_Commands_\n\n");
    for (int i = COMMANDS_SIZE; i--;)
      help_command(commands[i], flags & HELP_COMMANDS_FULL);
    PC('\n');
  }

  if (flags & HELP_KEYS) PA(
    "_Keys_\n\n"
    " *Symbolic keys*\n"
    "  Up/Down/Left/Right, PageUp/PageDown, Home/End, Insert/Delete,\n"
    "  Escape, Space, Enter, Tab, Backspace, F1 .. F12\n\n"
    " *Modifiers*\n"
    "  *Control*: Ctrl-key, C-key, ^key\n"
    "  *Alt*:     Alt-key, A-key, Meta-key, M-key\n"
    "  *Shift*:   Shift-key, S-key\n\n"
  );

  return 0;
}

// Returns [-fb] [-a arg] [-s arg]
static char* __inline get_option_string(const command_t *cmd ) {
  char ret[1024]      = { '\0' };
  char opts[128]      = { '\0' };
  char flags[128], *f = flags;

  if (cmd->opts) {
    for (command_opt_t *opt = cmd->opts; opt->opt; ++opt)
      if (opt->meta) {
        char buf[64];
        sprintf(buf, " [-%c %s]", opt->opt, opt->meta);
        strcat(opts, buf);
      }
      else
        *f++ = opt->opt;
    *f = '\0';

    if (strlen(flags))
      sprintf(ret, "[-%s]", flags);
    if (strlen(opts))
      strcat(ret, opts + !strlen(flags) /*Trim leading space*/);
  }

  return strdup(ret);
}

// Returns ARGUMENT [OPTIONAL...]
static char* __inline get_argument_string(const command_t *cmd) {
  char ret[1024] = { '\0' };

  if (cmd->args) {
    for (const char *arg = cmd->args; *arg; arg += (1+strlen(arg))) {
      char buf[64];
      if      (*arg == '+') sprintf(buf, " _%s_...",   arg + 1);
      else if (*arg == '*') sprintf(buf, " [_%s_...]", arg + 1);
      else                  sprintf(buf, " _%s_",      arg);
      strcat(ret, buf);
    }
  }

  return strdup(ret);
}

static void help_command(const command_t *cmd, int full) {
  if (! full) {
    PFA("*%-15s*", cmd->name);
    PA(firstline(GET_CMD_DESCRIPTION(cmd->name), 0));
    PC('\n');
    return;
  }

  PFA("*%s*", cmd->name);

  if (cmd->opts) {
    PC(' ');
    PA(get_option_string(cmd));
  }

  PA(get_argument_string(cmd));

  PC('\n');
  PA(indent(GET_CMD_DESCRIPTION(cmd->name), 1));
  PC('\n');

  if (cmd->opts) {
    PC('\n');

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

      const char *more_lines = 0;
      PA(firstline(opt->desc, &more_lines));
      if (more_lines)
        PA(indent(more_lines, 6 + max));
      PC('\n');
    }
  }

  PC('\n');
}

// Indent a block of lines
static char* indent(const char *str, int pad) {
  int  ind = -1;
  char res[8192];

  goto INDENT; // First indent does not depend on newline

  do {
    res[++ind] = *str;
    if (*str++ == '\n')
INDENT:
      for (int i = pad; i--;)
        res[++ind] = ' ';
  } while (*str);

  res[++ind] = '\0';
  return strdup(res);
}

// Replace pseudo markdown ("*", "_") with replacements
static char* fill_attrs(const char *s) {
  char res[8192] = { '\0' };
  int  state = 1;
  int  ind = 0;

  do {
    switch (*s) {
      case '*':   // index = 42 % 2 == 0
      case '_':   // index = 95 % 2 == 1
                  if (README || isatty(STDOUT_FILENO)) {
                    strcat(res, attrs[(2*(*s%2)) + (state = !state)]);
                    ind = strlen(res);
                  }
                  break;
      case '\\':  ++s; // fall-through
      default:    res[ind] = *s;
                  res[++ind] = '\0';
    }
  } while (*s && *++s);

  return strdup(res);
}

static char* firstline(const char *s, const char **other_lines) {
  char firstline[1024];
  char *f = firstline;
  while (*s && *s != '\n')
    *f++ = *s++;
  *f = '\0';
  if (other_lines && *s)
    *other_lines = s + 1;
  return strdup(firstline);
}

