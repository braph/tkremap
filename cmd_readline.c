#include "tkremap.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <pty.h>
#include <termios.h>
#include <readline/readline.h>

#define ACTION_WRITELINE 0
#define ACTION_EVALUATE  1
#define ACTION_CONFIRM   2

// readline: "> "
// confirm:  "? "
// shell:    "$ "
// tksh:     "(mode:)"

int read_conf_string(const char *); // conf.h

typedef struct cmd_readline_args {
  uint8_t do_newline    : 1;
  uint8_t no_clear      : 1;
  uint8_t no_refresh    : 1;
  uint8_t action        : 2;
  uint8_t __PAD__       : 3;
  int16_t x;
  int16_t y;
  char   *prompt;
  char   *init;
  char   *prepend;
  char   *append;
} cmd_readline_args;

static char* init_text;
extern rl_hook_func_t *rl_startup_hook;

// Find a solution to not display a '\n' when hitting Enter
/*
extern rl_getc_func_t *rl_getc_function;
int my_getc(FILE *f) {
  char c;
  set_input_mode();
  if (1 != read(STDIN_FILENO, &c, 1))
    abort();
  if (c != '\n')
    write(STDOUT_FILENO, &c, 1);
  return c;
} */

int rl_set_init_text(void) {
  //rl_reset_terminal(getenv("TERM"));
  if (init_text)
    rl_insert_text(init_text);
  return 0;
}

static void get_winsize(int fd, int *y, int *x) {
  struct winsize wsz;
  if (ioctl(fd, TIOCGWINSZ, &wsz) != -1)
    *y = wsz.ws_row, *x = wsz.ws_col;
  else
    *y = *x = 0;
}

static int confirm(char *prompt) {
  set_input_mode();

  if (prompt) {
    printf("%s", prompt);
    fflush(NULL);
  }

  for (int c = 0 ;; c = 0) {
    read(STDIN_FILENO, &c, 1);
    switch(c) {
      case 'y': case 'Y': return 1;
      case 'n': case 'N':
      case 0: /*EOF*/     return 0;
    }
  }
}

static COMMAND_CALL_FUNC(cmd_readline_call) {
  char *line = NULL;
  int old_x, old_y, max_x, max_y, x, y;
  int ret = 0;
  cmd_readline_args *args = cmd->arg;

  init_text       = args->init;
  rl_startup_hook = &rl_set_init_text;

  tcsetattr(STDOUT_FILENO, TCSANOW, &context.tios_restore);
#if !TESTMODE
  get_cursor(STDIN_FILENO, &old_y, &old_x);
  get_winsize(context.program_fd, &max_y, &max_x);

  if (args->x < 0) {
    x = max_x + (args->x + 1);
    x = (x < 1 ? 1 : x);
  }
  else if (args->x > 0)
    x = (args->x > max_x ? max_x : args->x);
  else
    x = old_x;

  if (args->y < 0) {
    y = max_y + (args->y + 1);
    y = (y < 1 ? 1 : y);
  }
  else if (args->y > 0)
    y = (args->y > max_y ? max_y : args->y);
  else
    y = old_y;

  // Save screen
  if (! args->no_refresh)
    redraw_begin();

  // Changing cursor line has been requested
  if (args->x || args->y)
    set_cursor(STDOUT_FILENO, y, x);

  // Clear line
  if (! args->no_clear)
    CLEAR_TO_EOL(STDOUT_FILENO);
#endif //!TESTMODE

  // Actions: CONFIRM, EVALUATE, WRITELINE
  if (args->action == ACTION_CONFIRM)
    ret = confirm(args->prompt);
  else if (args->action == ACTION_EVALUATE) {
    // Invoke readline with modified prompt
    char new_prompt[4096];

    snprintf(new_prompt, sizeof(new_prompt), "(mode: %s) ", context.current_mode->name);
    if (args->prompt)
      strcat(new_prompt, args->prompt);

    line = readline(new_prompt);
  }
  else { // ACTION_WRITELINE
    line = readline(args->prompt);
  }

  set_input_mode();

#if !TESTMODE
  // Erase line written by user
  set_cursor(STDOUT_FILENO, y, x);
  if (! args->no_clear)
    CLEAR_TO_EOL(STDOUT_FILENO);

  // Restore screen
  set_cursor(STDOUT_FILENO, old_y, old_x);

  if (! args->no_refresh)
    redraw_redraw();
#endif //!TESTMODE

  // Handle input
  if (line && strlen(line) > 0) {
    ret = 1; // Having line -> return Success
    char *modified_line = line;
    char *prepend  = (args->prepend    ? args->prepend :  "" );
    char *append   = (args->append     ? args->append  :  "" );
    char  nl       = (args->do_newline ? '\r'          : '\0');
    char  buffer[8192];

    if (args->action == ACTION_EVALUATE) {
      // Newline not needed in evaluation
      if (args->prepend || args->append) {
        snprintf(buffer, sizeof(buffer), "%s%s%s", prepend, line, append);
        modified_line = buffer;
      }

      if (! read_conf_string(modified_line)) {
        // If command string is invalid, re-invoke this command till death
        char *old_init = args->init;
        args->init = line;
        cmd_readline_call(cmd, key);
        args->init = old_init;
        free(line);
        return 1;
      }
    }
    else {
      if (args->prepend || args->append || args->do_newline) {
        snprintf(buffer, sizeof(buffer), "%s%s%s%c", prepend, line, append, nl);
        modified_line = buffer;
      }

      writes_to_program(modified_line);
    }
  }

  free(line);
  return ret;
}

static void cmd_readline_free(void *);
static COMMAND_PARSE_FUNC(cmd_readline_parse) {
  cmd_readline_args *cmd_args = calloc(1, sizeof(*cmd_args));
  cmd_args->x =  1;
  cmd_args->y = -2;

  for (option *opt = options; opt->opt; ++opt) {
    #define case break; case
    switch (opt->opt) {
      case 'n': cmd_args->do_newline    = 1;
      case 'R': cmd_args->no_refresh    = 1;
      case 'C': cmd_args->no_clear      = 1;
      case '!': cmd_args->action        = ACTION_EVALUATE;
      case '?': cmd_args->action        = ACTION_CONFIRM;
    //case '$': cmd_args->action        = ACTION_SHELL;
      case 'x': cmd_args->x             = atoi(opt->arg);
      case 'y': cmd_args->y             = atoi(opt->arg);
      case 'i': free(cmd_args->init);
                cmd_args->init = strdup(opt->arg);
      case 'p': free(cmd_args->prompt);
                cmd_args->prompt = strdup(opt->arg);
      case 'A': free(cmd_args->append);
                cmd_args->append = strdup(opt->arg);
      case 'P': free(cmd_args->prepend);
                cmd_args->prepend = strdup(opt->arg);
    }
    #undef case
  }

  return (void*) cmd_args;
}

static void cmd_readline_free(void *_arg) {
  cmd_readline_args *arg = (cmd_readline_args*) _arg;
  free(arg->init);
  free(arg->prompt);
  free(arg->append);
  free(arg->prepend);
  free(arg);
}

//.opts = "readline(-p PROMPT, -P TEXT + <INP> + ",
const command_t command_readline = {
  .name  = "readline"
    "\0Write to program using readline\n\n"
    "If input is empty *FALSE* is returned, otherwise *TRUE* is returned\n\n"
    "*TIP*: To append keys to resulting line use { read && { key ... } }\n\n"
    "See also *redraw\\_method*",
  .opts  = OPTIONS(
      OPTION('p', "PROMPT", "Set prompt text"),
      OPTION('i', "TEXT",   "Initialize buffer with _TEXT_"),
      OPTION('x', "X",      "Set X cursor position\n"
        "Position starts from left. Use a negative value to start from right.\n"
        "Use 0 for not changing the cursor position [*default*]"),
      OPTION('y', "Y",      "Set Y cursor position\n"
        "Position starts from top. Use a negative value to start from bottom.\n"
        "Use 0 for not changing the cursor position [*default*]"),
      OPTION('n', NO_ARG,   "Append a newline to result string"),
      OPTION('C', NO_ARG,   "Do not clear the cursor line"),
      OPTION('k', "KEY",    "Send _KEY_ after writing line"),
      OPTION('R', NO_ARG,   "Do not redraw the window (see *redraw\\_method*)"),
      OPTION('A', "TEXT",   "Append _TEXT_ to result"),
      OPTION('P', "TEXT",   "Prepend _TEXT_ to result"),
      OPTION('!', NO_ARG,   "Execute line as *tkremap* command"),
      OPTION('?', NO_ARG,   "Read *y*/*N* and return result as *TRUE*/*FALSE*")
  ),
  .parse = &cmd_readline_parse,
  .call  = &cmd_readline_call,
  .free  = &cmd_readline_free
};

