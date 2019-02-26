#include "tkremap.h"
#include "common.h"
#include "errormsg.h"
#include "termkeystuff.h"

#include <pty.h>
#include <poll.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

struct context_t   context;

static char* tkremap_error = NULL;
#define      TKREMAP_ERROR_MAX 2048

// Return the current error
char *get_error() {
   return tkremap_error;
}

// Start a new error
void write_error(const char *fmt, ...) {
   char temp[TKREMAP_ERROR_MAX];

   va_list ap;
   va_start(ap, fmt);
   vsnprintf(temp, TKREMAP_ERROR_MAX, fmt, ap);
   va_end(ap);

   free(tkremap_error);
   tkremap_error = strdup(temp);
}

// Append error message to an existing error separated by ": "
void prepend_error(const char *fmt, ...) {
   char *old_error = tkremap_error;
   char temp[TKREMAP_ERROR_MAX];

   va_list ap;
   va_start(ap, fmt);
   int l = vsnprintf(temp, TKREMAP_ERROR_MAX, fmt, ap);
   va_end(ap);

   tkremap_error = malloc(sizeof(": ") + strlen(old_error) + l);
   sprintf(tkremap_error, "%s: %s", temp, old_error);
   free(old_error);
}

void writeb_to_program(const char *s, ssize_t len) {
   ssize_t n;

   for (int i = 5; i--; ) {
      n = write(context.program_fd, s, len);
      if (n >= 0) {
         s += n;
         len -= n;
         if (len == 0)
            break;
      } else if (n == -1 && errno != EAGAIN)
         break;
      usleep(100);
   }
}

void writes_to_program(const char *s) {
   writeb_to_program(s, strlen(s));
}

void context_init() {
   context.keymodes        = NULL;
   context.n_keymodes      = 0;
   context.current_mode    = &context.global_mode;
   context.current_binding = NULL;
   context.mask            = 0;
   context.repeat          = 0;
   keymode_init(&context.global_mode,  "global",  UNBOUND_PASS);
   keymode_init(&context.default_mode, "default", UNBOUND_PASS);
}

#if FREE_MEMORY
void context_free() {
   keymode_free(&context.global_mode);

   for (int i = context.n_keymodes; i--; ) {
      keymode_free(context.keymodes[i]);
      free(context.keymodes[i]);
   }

   free(context.keymodes);
}
#endif


void keymode_init(keymode_t *km, const char *name, int unbound_action) {
   km->name             = strdup(name);
   km->unbound_unicode  = unbound_action;
   km->unbound_keysym   = unbound_action;
   km->unbound_function = unbound_action;
   km->unbound_mouse    = unbound_action;
   km->repeat_enabled   = 0;
   km->root             = malloc(sizeof(binding_root_t));
   km->root->size       = 0;
   km->root->p.bindings = NULL;
   km->root->type       = BINDING_TYPE_CHAINED;
}

keymode_t* get_keymode(const char *name) {
   if (streq(name, "global"))
      return &context.global_mode;

   if (streq(name, "default"))
      return &context.default_mode;

   for (int i = context.n_keymodes; i--;)
      if (streq(name, context.keymodes[i]->name))
         return context.keymodes[i];

   return NULL;
}

keymode_t* add_keymode(const char *name) {
   keymode_t *km = malloc(sizeof(*km));
   keymode_init(km, name, UNBOUND_IGNORE);

   context.n_keymodes++;
   context.keymodes = realloc(context.keymodes, context.n_keymodes * sizeof(km));
   context.keymodes[context.n_keymodes - 1] = km;
   return km;
}

binding_t*
binding_get_binding(binding_t *binding, TermKeyKey *key) {
   for (int i = binding->size; i--; )
      if (! termkey_keycmp(tk, key, &binding->p.bindings[i]->key))
         return binding->p.bindings[i];

   return NULL;
}

binding_t*
binding_add_binding(binding_t *binding, binding_t *next_binding) {
   binding->size++;
   binding->p.bindings = realloc(binding->p.bindings, binding->size * sizeof(binding_t*));
   return (binding->p.bindings[binding->size - 1] = next_binding);
}

void
binding_del_binding(binding_t *binding, TermKeyKey *key) {
   for (int i=0; i < binding->size; ++i)
      if (! termkey_keycmp(tk, key, &binding->p.bindings[i]->key)) {
         binding_free(binding->p.bindings[i]);
         for (++i; i < binding->size; ++i)
            binding->p.bindings[i - 1] = binding->p.bindings[i];
         break;
      }

   binding->size--;
   binding->p.bindings = realloc(binding->p.bindings, binding->size * sizeof(binding_t*));
}

#if FREE_MEMORY
void keymode_free(keymode_t *km) {
   binding_free(km->root);
   free(km->root);
   free(km->name);
}
#endif


void command_call_free(command_call_t *call) {
   if (call->command->free)
      call->command->free(call->arg);
}

void binding_free(binding_t *binding) {
   if (binding->type == BINDING_TYPE_COMMAND) {
      for (int i = binding->size; i--; )
         command_call_free(&binding->p.commands[i]);
   }
   else {
      for (int i = binding->size; i--; ) {
         binding_free(binding->p.bindings[i]);
         free(binding->p.bindings[i]);
      }
   }

   free(binding->p.commands);
   binding->p.commands = NULL;
   binding->size = 0;
}

void command_execute(command_call_t *cmd, TermKeyKey *key) {
   cmd->command->call(cmd, key);
}

void commands_execute(binding_t *binding, TermKeyKey *key) {
   for (int i=0; i < binding->size; ++i)
      command_execute(&binding->p.commands[i], key);
}

static
void commands_execute_with_repeat(binding_t *binding, keymode_t *km, TermKeyKey *key) {
   if (km->repeat_enabled && context.repeat) {
      for (int r = context.repeat; r--; )
         commands_execute(binding, key);
      context.repeat = 0;
   }
   else {
      commands_execute(binding, key);
   }
}

static
void binding_execute(binding_t *binding, keymode_t *km, TermKeyKey *key) {
   if (binding->type == BINDING_TYPE_COMMAND) {
      commands_execute_with_repeat(binding, km, key);
      context.current_binding = NULL;
   }
   else {
      context.current_binding = binding;
   }
}

int check_args(int argc, const char *args[]) {
   for (const char **arg = args; *arg; ++arg) {
      if (**arg == '+') {
         if (! argc) {
            write_error("%s: %s", E_MISSING_ARG, (*arg+1));
            return 0;
         }

         break; // OK
      }
      else if (**arg == '*') {
         break; // OK
      }
      else {
         if (! argc--) {
            write_error("%s: %s", E_MISSING_ARG, *arg);
            return 0;
         }
      }
   }

   return 1;
}

char* args_get_arg(int *argc, char ***argv, const char *name) {
   if (! *argc) {
      write_error("%s: %s", E_MISSING_ARG, name);
      return NULL;
   }

   char *ret = (*argv)[0];
   --(*argc), ++(*argv);
   return ret;
}

void handle_key(TermKeyKey *key) {
   binding_t *binding;
   int unbound_action = UNBOUND_PASS;

   // Masked mode =============================================================
   if (context.mask) {
      context.mask = 0;
      goto WRITE_RAW;
   }

   // We're in a keybinding-chain =============================================
   if (context.current_binding != NULL) {
      if ((binding = binding_get_binding(context.current_binding, key)))
         binding_execute(binding, context.current_mode, key);
      else {
         context.current_binding = NULL;

         if (key->type == TERMKEY_TYPE_KEYSYM || (key->type == TERMKEY_TYPE_UNICODE && key->modifiers))
            unbound_action = context.current_mode->unbound_keysym;
         else if (key->type == TERMKEY_TYPE_UNICODE)
            unbound_action = context.current_mode->unbound_unicode;
         else if (key->type == TERMKEY_TYPE_FUNCTION)
            unbound_action = context.current_mode->unbound_function;
         else if (key->type == TERMKEY_TYPE_MOUSE)
            unbound_action = context.current_mode->unbound_mouse;

         if (unbound_action == UNBOUND_IGNORE)
            return;
         else if (unbound_action == UNBOUND_REEVAL)
            return handle_key(key);
         else // UNBOUND_PASS
            goto WRITE_RAW;
      }
   }

   // Special case: If building command repetition, don't pass 0 as keybinding,
   // instead multiply our current repeat val by 10
   if (context.current_mode->repeat_enabled &&
       context.repeat > 0                   &&
       key->type == TERMKEY_TYPE_UNICODE    &&
       key->code.codepoint == '0') {
      context.repeat *= 10;
      return;
   }

   // === Try current_mode then global_mode ===================================
   keymode_t *keymode = context.current_mode;

   NEXT_KEYMODE:
   if ((binding = binding_get_binding(keymode->root, key))) {
      binding_execute(binding, keymode, key);
      return;
   }

   if (keymode != &context.global_mode) {
      keymode = &context.global_mode;
      goto NEXT_KEYMODE;
   }
   // =========================================================================

   // We have the chance to start a command repetition ========================
   if (context.current_mode->repeat_enabled) {
      if (key->type == TERMKEY_TYPE_UNICODE &&
          key->code.codepoint >= '1'        &&
          key->code.codepoint <= '9')
      {
         context.repeat = context.repeat * 10 + (key->code.codepoint - '0');
         return;
      }
      else
         context.repeat = 0; // no
   }

   // Handle unbound key ======================================================
   if (key->type == TERMKEY_TYPE_KEYSYM || (key->type == TERMKEY_TYPE_UNICODE && key->modifiers))
      unbound_action = context.current_mode->unbound_keysym;
   else if (key->type == TERMKEY_TYPE_UNICODE)
      unbound_action = context.current_mode->unbound_unicode;
   else if (key->type == TERMKEY_TYPE_FUNCTION)
      unbound_action = context.current_mode->unbound_function;
   else if (key->type == TERMKEY_TYPE_MOUSE)
      unbound_action = context.current_mode->unbound_mouse;

   if (unbound_action == UNBOUND_IGNORE || unbound_action == UNBOUND_REEVAL)
      return;

   WRITE_RAW:
   write(context.program_fd, context.input_buffer, context.input_len);
}

static
void *redirect_to_stdout(void *_fd)
{
   #define  REDIRECT_BUFSZ 4096
   int      fd = *((int*)_fd);
   char     buffer[REDIRECT_BUFSZ];
   char     *b;
   ssize_t  bytes_read;
   ssize_t  bytes_written;
   struct   pollfd fds = { .fd = fd, .events = POLLIN };

   for (;;) {
      if (poll(&fds, 1, 100) > 0 && fds.revents & POLLIN ) {
         if (context.stop_output)
            return NULL;
      }
      else {
         if (context.stop_output)
            return NULL;
         continue;
      }

      if ((bytes_read = read(fd, &buffer, REDIRECT_BUFSZ)) == -1) {
         if (errno == EAGAIN) {
            usleep(100);
            continue;
         }
         else {
            return NULL;
         }
      }

      b = buffer;
      for (int i = 5; i--; ) {
         bytes_written = write(STDOUT_FILENO, b, bytes_read);

         if (bytes_written >= 0) {
            b += bytes_written;
            bytes_read -= bytes_written;
            if (bytes_read == 0)
               break;
         }
         else if (bytes_written == -1 && errno != EAGAIN)
            break;

         usleep(100);
      }
   }
}

int start_program_output() {
   context.stop_output = 0;
   if ((errno = pthread_create(&context.redir_thread,
         NULL, redirect_to_stdout, (void*)&context.program_fd)))
      return 0;
   return 1;
}

void stop_program_output() {
   context.stop_output = 1;
   pthread_join(context.redir_thread, NULL);
}

void get_cursor(int fd, int *y, int *x) {
   // Send "\033[6n"
   // Expect ^[[8;14R
   fd = STDIN_FILENO;
   *x = *y = 0;
   struct termios tios, old_tios;
   char cmd[] = { 033, '[', '6', 'n' };
   char c;

   if (tcgetattr(fd, &tios) == 0) {
      old_tios = tios;
      cfmakeraw(&tios);
      tcsetattr(fd, TCSANOW, &tios);

      write(fd, cmd, sizeof(cmd));
      read(fd, cmd, 2); // ESC, [

      while (read(fd, &c, 1) && c != ';')
         *y = (*y * 10) + c - '0';

      while (read(fd, &c, 1) && c != 'R')
         *x = (*x * 10) + c - '0';

      tcsetattr(fd, TCSANOW, &old_tios);
   }
}

void set_cursor(int fd, int y, int x) {
   dprintf(fd, "\033[%d;%dH", y, x);
}

void set_input_mode() {
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

   tcgetattr(STDIN_FILENO, &tios);
   ioctl(STDIN_FILENO, TIOCGWINSZ, &wsz);
      
   *pid = forkpty(ptyfd, NULL, &tios, &wsz);

   if (*pid < 0)
      return -1;
   else if (*pid == 0) {
      execvp(argv[0], &argv[0]);
      return -1;
   }

   return 1;
}

void update_pty_size(int _) {
   struct winsize ws;
   if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != -1)
      ioctl(context.program_fd, TIOCSWINSZ, &ws);
}

const char *key_parse_get_code(const char *keydef) {
   TermKeyKey key;
   const char *seq;

   if (! parse_key(keydef, &key))
      return 0;

   if (! (seq = get_key_code(&key)))
      write_error("%s: %s", E_KEYCODE_NA, keydef);

   return seq; // is NULL if failed
}

