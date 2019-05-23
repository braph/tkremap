#include "tkremap.h"
#include "common.h"
#include "termkeystuff.h"

#include <pty.h>
#include <poll.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#define SET_CURSOR_Y_X "\033[%d;%dH"
#define REQUEST_CURSOR "\033[6n"
#define GLOBAL         "global"
#define DEFAULT        "default"

struct context_t context;

void writeb_to_program(const char *s, ssize_t len) {
  write_full(context.program_fd, s, len);
}

void writes_to_program(const char *s) {
  writeb_to_program(s, strlen(s));
}

void context_init() {
  context.global_mode.name  = GLOBAL;
  context.default_mode.name = DEFAULT;
  context.global_mode.root  = calloc(1, BINDING_T_ROOT_NODE_SIZE);
  context.default_mode.root = calloc(1, BINDING_T_ROOT_NODE_SIZE);

  context.current_mode = &context.default_mode;
  for (int i = MODESTACK_SIZE; i--;)
    context.modestack[i] = &context.default_mode;
}

#if FREE_MEMORY
void context_free() {
  // We cannot free constant strings
  context.global_mode.name  = NULL;
  context.default_mode.name = NULL;
  keymode_free(&context.global_mode);
  keymode_free(&context.default_mode);

  for (int i = context.keymodes_size; i--;) {
    keymode_free(context.keymodes[i]);
    free(context.keymodes[i]);
  }

  free(context.keymodes);
}
#endif

keymode_t* get_keymode(const char *name) {
  if (streq(name, GLOBAL))
    return &context.global_mode;

  if (streq(name, DEFAULT))
    return &context.default_mode;

  for (int i = context.keymodes_size; i--;)
    if (streq(name, context.keymodes[i]->name))
      return context.keymodes[i];

  error_set_errno(E_UNKNOWN_MODE);
  return NULL;
}

keymode_t* add_keymode(const char *name) {
  keymode_t *km = calloc(1, sizeof(*km));
  km->name = strdup(name);
  km->root = calloc(1, BINDING_T_ROOT_NODE_SIZE);

  context.keymodes_size++;
  context.keymodes = realloc(context.keymodes, context.keymodes_size * sizeof(km));
  context.keymodes[context.keymodes_size - 1] = km;
  return km;
}

binding_t* binding_get_binding(binding_t *binding, TermKeyKey *key) {
  for (int i = binding->size; i--;)
    if (! termkey_keycmp(tk, key, &binding->bindings[i]->key))
      return binding->bindings[i];

  return NULL;
}

binding_t* binding_add_binding(binding_t *binding, binding_t *next_binding) {
  binding->size++;
  binding->bindings = realloc(binding->bindings, binding->size * sizeof(binding_t*));
  return (binding->bindings[binding->size - 1] = next_binding);
}

#if FREE_MEMORY
void keymode_free(keymode_t *km) {
  for (int i = 0; i < 4; ++i)
    if (km->unbound[i] != NULL && PTR_UNREF(km->unbound[i])) {
      command_call_free(km->unbound[i]);
      free(km->unbound[i]);
    }

  binding_free(km->root);
  free(km->name);
}
#endif

// completely destroy a binding, including sub bindings
void binding_free(binding_t *binding) {
  if (binding->command != NULL && PTR_UNREF(binding->command)) {
    command_call_free(binding->command);
    free(binding->command);
  }

  for (int i = binding->size; i--;) {
    binding_free(binding->bindings[i]);
  }

  free(binding->bindings);
  free(binding);
}

void command_call_free(command_call_t *call) {
  if (call->command->free != NULL)
    call->command->free(call->arg);
}

int command_call_execute(command_call_t *call, TermKeyKey *key) {
  return call->command->call(call, key);
}

static
void command_call_execute_with_repeat(command_call_t *call, keymode_t *km, TermKeyKey *key) {
  if (km->repeat_enabled && context.repeat) {
    for (int r = context.repeat; r--;)
      command_call_execute(call, key);
    context.repeat = 0;
  }
  else
    command_call_execute(call, key);
}

static
void binding_execute(binding_t *binding, keymode_t *km, TermKeyKey *key) {
  //debug("binding_execute(..., %s, %s)", km->name, format_key(&binding->key));
  if (binding->command != NULL) {
    //debug("executing command");
    command_call_execute_with_repeat(binding->command, km, key);
  }

  if (binding->size > 0) {
    //debug("chain has more bindings");
    context.current_binding = binding;
  }
  else {
    //debug("chain has NO more bindings");
    context.current_binding = NULL;
  }
}

int check_args(int argc, const char *wanted_args) {
  for (const char *arg = wanted_args; *arg; arg += (1+strlen(arg))) {
    if (*arg == '+') {
      if (! argc)
        return error_set(E_MISSING_ARG, (arg+1)), 0;

      break; // OK
    }
    else if (*arg == '*') {
      break; // OK
    }
    else {
      if (! argc--)
        return error_set(E_MISSING_ARG, arg), 0;
    }
  }

  return 1;
}

int handle_key(TermKeyKey *key) {
  debug("handle_key(%s)", format_key(key));
  binding_t *binding;

  // Masked mode =============================================================
  if (context.mask) {
    context.mask = 0;
    goto WRITE_RAW;
  }

  // We're in a keybinding-chain =============================================
  if (context.current_binding != NULL) {
    //debug("context.current_binding->key = %s", format_key(&context.current_binding->key));
    if ((binding = binding_get_binding(context.current_binding, key))) {
      binding_execute(binding, context.current_mode, key);
      return 1;
    }
    else {
      //debug("no further binding found");
      context.current_binding = NULL;

      int keytype = key->type;
      if (keytype == TERMKEY_TYPE_UNICODE && key->modifiers)
        keytype = TERMKEY_TYPE_KEYSYM; // treat Ctrl/Alt as keysym

      if (context.current_mode->unbound[keytype])
        command_call_execute(context.current_mode->unbound[keytype], key);

      return 1;
    }

    return 0;
  }

  // Special case: If building command repetition, don't pass 0 as keybinding,
  // instead multiply our current repeat val by 10
  if (context.current_mode->repeat_enabled &&
      context.repeat > 0                   &&
      key->type == TERMKEY_TYPE_UNICODE    &&
      key->code.codepoint == '0') {
    context.repeat *= 10;
    return 1;
  }

  // === Try current_mode then global_mode ===================================
  if (context.current_mode == &context.global_mode) // This should actually
    context.current_mode = &context.default_mode;   // never happen

  if ((binding = binding_get_binding(context.current_mode->root, key)))
    return binding_execute(binding, context.current_mode, key), 1;

  if ((binding = binding_get_binding(context.global_mode.root, key)))
    return binding_execute(binding, &context.global_mode, key), 1;
  // =========================================================================

  // We have the chance to start a command repetition ========================
  if (context.current_mode->repeat_enabled) {
    if (key->type == TERMKEY_TYPE_UNICODE &&
        key->code.codepoint >= '1'        &&
        key->code.codepoint <= '9')
    {
      context.repeat = context.repeat * 10 + (key->code.codepoint - '0');
      return 1;
    }
    else
      context.repeat = 0; // no
  }

  // Handle unbound key ======================================================
  int keytype = key->type;
  if (keytype == TERMKEY_TYPE_UNICODE && key->modifiers)
    keytype = TERMKEY_TYPE_KEYSYM; // treat Ctrl/Alt + [a-z..] as KEYSYM

  if (context.current_mode->unbound[keytype]) {
    command_call_execute(context.current_mode->unbound[keytype], key);
    return 1;
  }

WRITE_RAW:
  write_full(context.program_fd, context.input_buffer, context.input_len);
  return 1;
}

void get_cursor(int fd, int *y, int *x) {
  *x = *y = 0;
  struct termios tios, old_tios;
  char c;
  char buf[3];

  if (tcgetattr(fd, &tios) == 0) {
    old_tios = tios;
    set_input_mode();                 // New Version
    //cfmakeraw(&tios);               // Old Version 
    //tcsetattr(fd, TCSANOW, &tios);  // ```````````

    // Write request
    write(fd, REQUEST_CURSOR, sizeof(REQUEST_CURSOR)-1);

    // Expect something like ^[[8;14R
    // -> 'ESC', '[', <N>, ';', <N>, 'R'
    read(fd, &buf, 3);

    *y = buf[2] - '0';
    while (read(fd, &c, 1) && c != ';')
      *y = (*y * 10) + c - '0';

    while (read(fd, &c, 1) && c != 'R')
      *x = (*x * 10) + c - '0';

    tcsetattr(fd, TCSANOW, &old_tios);
  }
}

void set_cursor(int fd, int y, int x) {
  char buffer[16];  // dprintf(fd, SET_CURSOR_Y_X, y, x);
  snprintf(buffer, sizeof(buffer), SET_CURSOR_Y_X, y, x);
  write_full(fd, buffer, strlen(buffer));
}

void set_input_mode() {
  // This function is like cfmakeraw()
  struct termios tios = context.tios_restore;
  tios.c_iflag    |= IGNBRK;
  tios.c_iflag    &= ~(IXON|INLCR|ICRNL);
  tios.c_lflag    &= ~(ICANON|ECHO|ECHONL|ISIG);
  tios.c_oflag    &= ~(OPOST|ONLCR|OCRNL|ONLRET);
  tios.c_cc[VMIN]  = 1;
  tios.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &tios);
}

int forkapp(char **argv, int *ptyfd, pid_t *pid) {
  struct winsize wsz;
  struct termios tios;

  if (isatty(STDOUT_FILENO)) {
    tcgetattr(STDOUT_FILENO, &tios);
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsz);
    *pid = forkpty(ptyfd, NULL, &tios, &wsz);
  }
  else {
    *pid = forkpty(ptyfd, NULL, NULL, NULL);
  }

  if (*pid < 0) {
    error_set_errno(errno);
    return 0;
  }
  else if (*pid == 0) {
    execvp(argv[0], &argv[0]);
    error_set_errno(errno);
    return 0;
  }

  return 1;
}

void update_pty_size(int sig) {
  struct winsize ws;
  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != -1)
    ioctl(context.program_fd, TIOCSWINSZ, &ws);
}

void refresh_window() {
  int fd = context.program_fd;
  struct winsize wsz;

  if (ioctl(fd, TIOCGWINSZ, &wsz) != -1) {
    wsz.ws_col--, ioctl(fd, TIOCSWINSZ, &wsz);
    kill(context.program_pid, SIGWINCH);
    usleep(250);
    wsz.ws_col++, ioctl(fd, TIOCSWINSZ, &wsz);
    kill(context.program_pid, SIGWINCH);
  }
}

const char *key_parse_get_code(const char *keydef) {
  TermKeyKey key;
  if (! parse_key(keydef, &key))
    return 0;

  return get_key_code(&key);
}

void redraw_begin() {
  if (context.redraw_method == REDRAW_METHOD_SRMCUP)
    SAVE_TERM_SCREEN(STDOUT_FILENO);
}

void redraw_redraw() {
  if (context.redraw_method == REDRAW_METHOD_SRMCUP)
    RESTORE_TERM_SCREEN(STDOUT_FILENO);
  else if (context.redraw_method == REDRAW_METHOD_RESIZE)
    refresh_window();
  else
    writes_to_program(context.redraw_method);
}

