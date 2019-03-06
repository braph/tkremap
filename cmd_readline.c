#include "tkremap.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <pty.h>
#include <termios.h>
#include <readline/readline.h>

#define SMCUP     "\033[?1049h"
#define RMCUP     "\033[?1049l"
#define CLEARLINE "\033[K"
#define STRLEN(S) (sizeof(S) - 1)

int read_conf_string(const char *); // conf.h

typedef struct cmd_readline_args { // cmd_readline.c + cmd_command.c
  uint8_t do_newline       : 1;
  uint8_t do_clear         : 1;
  uint8_t do_refresh       : 1;
  uint8_t do_config        : 1;
  uint8_t do_savescreen    : 1;
  uint8_t is_redisplayed   : 1;
  uint8_t __PAD__          : 2;
  int16_t x;
  int16_t y;
  char   *prompt;
  char   *init;
  char   *keyseq;
  char   *prepend;
  char   *append;
} cmd_readline_args;

static cmd_readline_args *current_arg;
extern rl_hook_func_t *rl_startup_hook;

int rl_set_init_text(void) {
  rl_reset_terminal(getenv("TERM"));
  if (current_arg->init)
    rl_insert_text(current_arg->init);
  return 0;
}

static
void get_winsize(int fd, int *y, int *x) {
  struct winsize wsz;
  if (ioctl(fd, TIOCGWINSZ, &wsz) != -1)
    *y = wsz.ws_row, *x = wsz.ws_col;
}

static
void refresh_window(int fd) {
  struct winsize wsz;
  if (ioctl(fd, TIOCGWINSZ, &wsz) != -1) {
    wsz.ws_col--, ioctl(fd, TIOCSWINSZ, &wsz);
    wsz.ws_col++, ioctl(fd, TIOCSWINSZ, &wsz);
  }
}

COMMAND_CALL_FUNC(cmd_readline_call) {
  int old_x, old_y, max_x = 0, max_y = 0, x, y;
  char *line;
  cmd_readline_args *args = cmd->arg;

  rl_startup_hook = &rl_set_init_text;
  current_arg = args;

  stop_program_output();
  tcsetattr(STDOUT_FILENO, TCSANOW, &context.tios_restore);
  get_cursor(context.program_fd, &old_y, &old_x);
  get_winsize(context.program_fd, &max_y, &max_x);

  if (args->x < 0) {
    x = max_x + (args->x + 1);
    if (x < 1)
      x = 1;
  }
  else if (args->x > 0)
    x = (args->x > max_x ? max_x : args->x);
  else
    x = old_x;

  if (args->y < 0) {
    y = max_y + (args->y + 1);
    if (y < 1)
      y = 1;
  }
  else if (args->y > 0)
    y = (args->y > max_y ? max_y : args->y);
  else
    y = old_y;

  if (args->do_savescreen)
    write(STDOUT_FILENO, SMCUP, STRLEN(SMCUP));

  set_cursor(STDOUT_FILENO, y, x);

  if (args->do_clear)
    write(STDOUT_FILENO, CLEARLINE, STRLEN(CLEARLINE));

  line = readline(args->prompt);

  set_input_mode();
  start_program_output();
  set_cursor(STDOUT_FILENO, old_y, old_x);

  if (line && strlen(line) > 0) {
    if (args->do_config) {
      if (! read_conf_string(line)) {
        // if string is invalid re-invoke command 
        char *old_init = args->init;
        args->init = line;
        cmd_readline_call(cmd, key);
        args->init = old_init;
        free(line);
        return 1;
      }
    }
    else {
      if (args->prepend)
        writes_to_program(args->prepend);
      writes_to_program(line);
      if (args->append)
        writes_to_program(args->append);
      if (args->do_newline)
        write(context.program_fd, "\r", 1);
    }
    free(line);
  }

  if (args->do_savescreen)
    write(STDOUT_FILENO, RMCUP, STRLEN(RMCUP));
  if (args->do_refresh)
    refresh_window(context.program_fd);
  if (args->keyseq)
    writes_to_program(args->keyseq);

  return 1;
}

void cmd_readline_free(void *);
COMMAND_PARSE_FUNC(cmd_readline_parse) {
  cmd_readline_args *cmd_args = calloc(1, sizeof(*cmd_args));
  cmd_args->x = 1;
  cmd_args->y = -2;

  for (option *opt = options; opt->opt; ++opt) {
    #define case break; case
    switch (opt->opt) {
      case 'n': cmd_args->do_newline    = 1;
      case 'c': cmd_args->do_clear      = 1;
      case 'r': cmd_args->do_refresh    = 1;
      case 'C': cmd_args->do_config     = 1;
      case 's': cmd_args->do_savescreen = 1;
      case 'p': 
          if (cmd_args->prompt)
            free(cmd_args->prompt);
          cmd_args->prompt = strdup(opt->arg);
      case 'i':
          if (cmd_args->init)
            free(cmd_args->init);
          cmd_args->init = strdup(opt->arg);
      case 'A': 
          if (cmd_args->append)
            free(cmd_args->append);
          cmd_args->append = strdup(opt->arg);
      case 'P':
          if (cmd_args->prepend)
            free(cmd_args->prepend);
          cmd_args->prepend = strdup(opt->arg);
      case 'k':
          if (! (cmd_args->keyseq = (char*) key_parse_get_code(opt->arg)))
            goto ERROR;

          cmd_args->keyseq = strdup(cmd_args->keyseq);
      case 'x':
          cmd_args->x = atoi(opt->arg);
      case 'y':
          cmd_args->y = atoi(opt->arg);
    }
    #undef case
  }

  return (void*) cmd_args;

ERROR:
  cmd_readline_free(cmd_args);
  return NULL;
}

void cmd_readline_free(void *_arg) {
  cmd_readline_args *arg = (cmd_readline_args*) _arg;
  free(arg->init);
  free(arg->prompt);
  free(arg->keyseq);
  free(arg->append);
  free(arg->prepend);
  free(arg);
}

const command_t command_readline = {
  .name  = "readline",
  .desc  = 
    "Write to program using readline\n\n"
    "(*1*) The program's window content is refreshed by resizing its PTY.\n"
    "You may also try the *-k* option to send a key (e.g. *C-l*) for refreshing\n"
    "the screen instead (this may be more appropriate).",
  .opts  = (const command_opt_t[]) {
    {'p', "PROMPT", "Set prompt text"},
    {'i', "TEXT",   "Pre fill buffer with _TEXT_"},
    {'x', "X",      "Set x cursor position\n"
                    "Position starts from left. Use a negative value to start from right.\n"
                    "Use 0 for not changing the cursor position."},
    {'y', "Y",      "Set y cursor position\n"
                    "Position starts from top. Use a negative value to start from bottom.\n"
                    "Use 0 for not changing the cursor position."},
  //{'b', NULL,     "Set cursor to the bottom left (alias for -x 1 -y -1)"},
    {'n', NULL,     "Append a newline to result string"},
    {'k', "KEY",    "Send _KEY_ after writing line"},
    {'c', NULL,     "Clear the cursor line"},
    {'r', NULL,     "Refresh the window after readline call (*1*)"},
    {'s', NULL,     "Save the window using smcup/rmcup (see *tput*(1))"},
    {'A', "TEXT",   "Append _TEXT_ to result"},
    {'P', "TEXT",   "Prepend _TEXT_ to result"},
    {0,0,0}
  },
  .parse = &cmd_readline_parse,
  .call  = &cmd_readline_call,
  .free  = &cmd_readline_free
};

