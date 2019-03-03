#include "tkremap.h"
#include "errormsg.h"
#include "vi_conf.h"
#include "conf.h"

#include <err.h>
#include <poll.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>

#define USAGE \
"Usage: *%s* [_OPTIONS_] PROGRAM [ARGUMENTS...]\n\n"             \
"_OPTIONS_\n\n"                                                  \
" *-C* _STRING_\t"        "Read config string\n"                 \
" *-c* _FILE_\t"          "Read config file (see -h load)\n"     \
" *-m* _MODE_\t"          "Switch to _MODE_\n"                   \
" *-b* _KEY_ _CMD_\t"     "Alias for 'bind _KEY_ _CMD_'\n"       \
" *-k* _IN_ _OUT_\t"      "Alias for 'bind _IN_ key _OUT_'\n"    \
" *-u* _KEY_\t\t"         "Alias for 'unbind _KEY_\n"            \
" *-v*\t\t"               "Load builtin vi config\n"             \
"\n" \
"For more help:\n"                                               \
" *-h* _COMMAND_\n"                                              \
" *-h* commands\n"                                               \
" *-h* keys\n"                                                   \
" *-h* all\n\n"
#define GETOPT_OPTS "+c:C:m:b:k:hvu:"

void   cleanup();
void   sighandler(int);
void   tmux_fix();
char*  alias(const char*, ...);
int    load_conf(const char *);                       // cmd_load.c
int    help(const char *, const char*, const char *); // help.c

int main(int argc, char *argv[]) {
   int  c;
   char *arg2 = NULL;

   context_init();

   if (! load_terminfo())
      errx(1, "Could not setup terminfo");

   if (! (tk = termkey_new_abstract(getenv("TERM"), 0)))
      err(1, "Initializing termkey failed");

   while ((c = getopt(argc, argv, GETOPT_OPTS)) != -1)
      #define ERR(FMT, ...) errx(1, "Option -%c" FMT, c, __VA_ARGS__)
      #define case break; case
      switch (c) {
      case 'h':
         return help(argv[0], USAGE, argv[optind]);
      case 'm':
         if (! (context.current_mode = get_keymode(optarg)))
            ERR(": unknown mode: %s", optarg);
      case 'c':
         if (! load_conf(optarg))
            ERR(" '%s': %s", optarg, get_error());
      case 'C': 
         if (! read_conf_string(optarg))
            ERR(" '%s': %s", optarg, get_error());
      case 'b':
         if (! read_conf_string(alias("bind %s", optarg)))
            ERR(" '%s': %s", optarg, get_error());
      case 'u':
         if (! read_conf_string(alias("unbind %s", optarg)))
            ERR(" '%s': %s", optarg, get_error());
      case 'k':
         if ((arg2 = argv[optind++]) == NULL)
            ERR(" '%s': %s", optarg, E_MISSING_ARG);
         if (! read_conf_string(alias("bind %s key %s", optarg, arg2)))
            ERR(" '%s' '%s': %s", optarg, arg2, get_error());
      case 'v':
         if (! read_conf_string(VI_CONF))
            ERR(": %s", get_error());
      case '?':
         return 1;
      }
      #undef case
   alias(NULL); // free

   if (optind == argc)
      errx(1, "%s: command", E_MISSING_ARG);

   if (forkapp(&argv[optind], &context.program_fd, &context.program_pid) < 0)
      err(1, "Could not start process");

   if (! start_program_output())
      err(1, "Starting thread failed");

   if (tcgetattr(STDIN_FILENO, &context.tios_restore) != 0)
      err(1, "tcgetattr()");

   set_input_mode();
   tmux_fix();

   atexit(cleanup);
   signal(SIGINT,   sighandler);
   signal(SIGTERM,  sighandler);
   signal(SIGWINCH, update_pty_size);
   setbuf(stdin, NULL);

   #define ESCDELAY_MS 10
   struct pollfd fds[2] = {
      { .fd = STDIN_FILENO,       .events = POLLIN },
      { .fd = context.program_fd, .events = POLLIN }
   };
   TermKeyKey key;
   TermKeyKey escape = {
      .type      = TERMKEY_TYPE_KEYSYM,
      .code.sym  = TERMKEY_SYM_ESCAPE,
      .modifiers = 0
   };

   for (;;) {
      for (;;) {
         poll(fds, 2, -1);

         if (fds[1].revents & POLLHUP || fds[1].revents & POLLERR) {
            #define return_val c
            waitpid(context.program_pid, &return_val, 0);
            return WEXITSTATUS(return_val);
         }

         if (fds[0].revents & POLLIN)
            break;
      }

      c = getchar();
      context.input_buffer[context.input_len++] = c;

      if (c == 033) {
         if (poll(fds, 1, ESCDELAY_MS) > 0 && fds[0].revents & POLLIN)
            goto NON_ESCAPE;
         else {
            handle_key(&escape);
            context.input_len = 0;
            continue;
         }
      }

      NON_ESCAPE:
      termkey_push_bytes(tk, (char*) &c, 1);
      if (termkey_getkey(tk, &key) == TERMKEY_RES_KEY) {
         handle_key(&key);
         context.input_len = 0;
      }
   }

   return 0;
}

char* alias(const char *template, ...) {
   static char* buf = 0;
   if (! template) {
      free(buf);
      return 0;
   }

   int sz = strlen(template);
   va_list ap;
   va_start(ap, template);
   for (const char *s = strchr(template, '%'); s; s = strchr(s + 1, '%'))
      sz += strlen(va_arg(ap, char*));
   va_end(ap);

   buf = realloc(buf, sz);
   va_start(ap, template);
   vsprintf(buf, template, ap);
   va_end(ap);

   return buf;
}

void cleanup() {
   tcsetattr(STDIN_FILENO, TCSANOW, &context.tios_restore);
#if FREE_MEMORY
   termkey_destroy(tk);
   context_free();
   unload_terminfo();
   stop_program_output();
   pthread_join(context.redir_thread, NULL);
#endif
   exit(0);
}

void sighandler(int sig) {
   signal(sig, SIG_DFL);
   exit(0);
}

void tmux_fix() {
   if (getenv("TMUX") && fork() == 0) {
      close(0); close(1); close(2);
      execlp("tmux", "tmux", "setw", "escape-time", "50", NULL);
   }
}

