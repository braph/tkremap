#include "tkremap.h"
#include "aliases.h"
#include "vi_conf.h"
#include "conf.h"

#include <poll.h>
#include <sys/wait.h>

#define USAGE \
 "Usage: *%s* [_OPTIONS_] PROGRAM [_ARGUMENTS_...]\n\n"          \
 "_OPTIONS_\n\n"                                                 \
 " *-C* _STRING_\t"       "Read config string\n"                 \
 " *-c* _FILE_\t"         "Read config file (see -h load)\n"     \
 " *-m* _MODE_\t"         "Switch to _MODE_\n"                   \
 " *-b* _KEY_ _CMD_\t"    "Alias for 'bind _KEY_ _CMD_'\n"       \
 " *-k* _IN_ _OUT_\t"     "Alias for 'bind _IN_ key _OUT_'\n"    \
 " *-u* _KEY_\t\t"        "Alias for 'unbind _KEY_'\n"           \
 " *-v*\t\t"              "Load builtin vi keymap\n"             \
 "\n" \
 "For more help:\n"                                              \
 " *-h* _COMMAND_\n"                                             \
 " *-h* commands\n"                                              \
 " *-h* keys\n"                                                  \
 " *-h* all\n\n"
#define GETOPT_OPTS "+c:C:m:b:k:hvu:"

#define alias(...) \
  (snprintf(buffer, sizeof(buffer), __VA_ARGS__), buffer)

void cleanup();
void sighandler(int);
int  load_conf(const char*);                      // cmd_load.c
int  help(const char*, const char*, const char*); // help.c
void tmux_fix();

int main(int argc, char *argv[]) {
  char c;
  int  ok = 1;
  char *arg2;
  char buffer[8192];

  context_init();

  if (! load_terminfo()) {
    error_set(E_ERROR, "Could not setup terminfo");
    goto ERROR;
  }
  if (! (tk = termkey_new_abstract(getenv("TERM"), 0))) {
    error_set(E_ERROR, "Initializing termkey failed");
    goto ERROR;
  }

  // Default behaviour: Pass every key, suspend on ^Z
  read_conf_string("mo global;bi ^Z su");

  while ((c = getopt(argc, argv, GETOPT_OPTS)) != -1) {
    #define case break;case
    switch (c) {
      case '?': return E_UNKNOWN_OPT;
      case 'h': return help(argv[0], USAGE, argv[optind]);
      case 'm': ok = (uintptr_t) (context.current_mode = get_keymode(optarg));
      case 'c': ok = load_conf(optarg);
      case 'C': ok = read_conf_string(optarg);
      case 'u': ok = read_conf_string(alias("unbind %s", optarg));
      case 'v':
        if ((ok = read_conf_string(VI_CONF)))
          ok = (uintptr_t) (context.current_mode = get_keymode("vi"));
      case 'b':
        if (! (arg2 = argv[optind++]))
          ok = 0, error_set_errno(E_MISSING_ARG);
        else
          ok = read_conf_string(alias("bind %s %s", optarg, arg2));
      case 'k':
        if (! (arg2 = argv[optind++]))
          ok = 0, error_set_errno(E_MISSING_ARG);
        else
          ok = read_conf_string(alias("bind %s key %s", optarg, arg2));
    }
    #undef case

    if (! ok) {
      snprintf(buffer, sizeof(buffer), "Option -%c", c);
      error_add(buffer);
      goto ERROR;
    }
  }

  if (optind == argc) {
    error_set(E_MISSING_ARG, "PROGRAM");
    goto ERROR;
  }
  if (! forkapp(&argv[optind], &context.program_fd, &context.program_pid)) {
    error_add(argv[optind]);
    goto ERROR;
  }
  if (isatty(STDIN_FILENO)) {
    if (tcgetattr(STDIN_FILENO, &context.tios_restore) != 0) {
      error_set_errno(errno);
      goto ERROR;
    }
  }

  tmux_fix();
  set_input_mode();

  atexit(cleanup);
  signal(SIGINT,   sighandler);
  signal(SIGTERM,  sighandler);
  signal(SIGWINCH, update_pty_size);

  #define ESCDELAY_MS 10
  #define POLL_STDIN  0
  #define POLL_PROG   1
  #define return_val  ok
  #define nread       ok
  TermKeyKey key;
  struct pollfd fds[2] = {
    { .fd = STDIN_FILENO,       .events = POLLIN },
    { .fd = context.program_fd, .events = POLLIN }
  };

  for (;;) {
    for (;;) {
      poll(fds, 2, -1);

      if (fds[POLL_PROG].revents & POLLIN) {
        nread = read(context.program_fd, buffer, sizeof(buffer));
        if (nread > 0)
          write_full(STDOUT_FILENO, buffer, nread);
      }
      else if (fds[POLL_STDIN].revents & POLLIN) {
        break;
      }
      else if (fds[POLL_STDIN].revents & POLLERR || fds[POLL_STDIN].revents & POLLHUP) {
        // Our STDIN got closed?
        close(context.program_fd);
        waitpid(context.program_pid, &return_val, 0);
        return WEXITSTATUS(return_val);
      }
      else if (waitpid(context.program_pid, &return_val, WNOHANG) > 0) {
        // Program may have died
        return WEXITSTATUS(return_val);
      }
    }

    if (read(STDIN_FILENO, &c, 1) != 1)
      continue;

    context.input_buffer[context.input_len++] = c;

    if (c == 033) {
      if (poll(fds, 1, ESCDELAY_MS) > 0 && fds[POLL_STDIN].revents & POLLIN)
        goto NON_ESCAPE;
      else {
        // Pass Escape-key
        key.type      = TERMKEY_TYPE_KEYSYM;
        key.code.sym  = TERMKEY_SYM_ESCAPE;
        key.modifiers = 0;
        handle_key(&key);
        context.input_len = 0;
        continue;
      }
    }

    NON_ESCAPE:
    termkey_push_bytes(tk, (char*) &c, 1);
    if (termkey_getkey(tk, &key) == TERMKEY_RES_KEY) {
      if (key.type == TERMKEY_TYPE_KEYSYM && key.code.sym == TERMKEY_SYM_DEL)
        key.code.sym = TERMKEY_SYM_BACKSPACE; // remap DEL to BACKSPACE
      handle_key(&key);
      context.input_len = 0;
    }
  }

  return 0;

ERROR:
  dup2(STDERR_FILENO, STDOUT_FILENO);
  printf("%s: %s\n", argv[0], error_get());
  return error_get_errno();
}

void cleanup() {
  ___("cleanup");
  tcsetattr(STDIN_FILENO, TCSANOW, &context.tios_restore);
#if FREE_MEMORY
  termkey_destroy(tk);
  unload_terminfo();
  context_free();
  error_free();
#endif
}

void sighandler(int sig) {
  ___("sighandler");
  signal(sig, SIG_DFL);
  exit(sig == SIGINT ? 130 : 0);
}

void tmux_fix() {
  if (getenv("TMUX") && fork() == 0) {
    close(0), close(1), close(2);
    execlp("tmux", "tmux", "setw", "escape-time", "50", NULL);
  }
}

