#ifndef _TKREMAP_H
#define _TKREMAP_H

#include "common.h"
#include "conf.h"
#include "options.h"
#include "errormsg.h"
#include "termkeystuff.h"

#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <termios.h>
#include <termkey.h>
#include <sys/types.h>

struct context_t;
struct binding_t;
struct command_call_t;

// === Global variables ===
extern struct context_t context;
// ========================

// === Command stuff ==========================================================
#define OPTIONS(...) \
  (const command_opt_t[]) { __VA_ARGS__, {0} }

#define OPTION(FLAG, META, DESC) \
  { FLAG, META, DESC }

#define NO_ARG NULL

#define ARGUMENTS(...) \
  (const char*[]) { __VA_ARGS__, NULL }

#define COMMAND_CALL_FUNC(NAME) \
  int   NAME(struct command_call_t* cmd, TermKeyKey *key)

#define COMMAND_PARSE_FUNC(NAME) \
  void* NAME(int argc, char *args[], option *options, const command_t **switch_command)

typedef const struct command_opt_t {
  const char  opt;
  const char *meta;
  const char *desc;
} command_opt_t;

typedef struct command_t {
  const char     *name;
  const char     *args;
  command_opt_t  *opts;
  void*         (*parse) (int, char **, option*, const struct command_t**);
  int           (*call)  (struct command_call_t*, TermKeyKey*);
  void          (*free)  (void*);
} command_t;

typedef struct command_call_t {
  const command_t  *command;
  void             *arg;
  int               refcount;
} command_call_t;

void command_call_free(command_call_t *);
int  command_call_execute(command_call_t *, TermKeyKey *);

// === Bindings ===============================================================
typedef struct binding_t {
  struct binding_t  **bindings;
  int                 size;
  command_call_t     *command;
  TermKeyKey          key; // At the bottom, important!
} binding_t;

// Field `key` is not used in the root node - leave it out.
#define BINDING_T_ROOT_NODE_SIZE \
  (sizeof(binding_t) - sizeof(TermKeyKey))

void       binding_free(binding_t *);
binding_t* binding_get_binding(binding_t *, TermKeyKey *);
binding_t* binding_add_binding(binding_t *, binding_t *);
// ============================================================================

typedef struct __packed keymode_t {
  char            *name;
  command_call_t  *unbound[4];
  binding_t       *root;
  uint8_t         repeat_enabled;
} keymode_t;

void keymode_free(keymode_t*);

#define REHANDLE_DEPTH_MAX    10
#define MODESTACK_SIZE        1 << 3
#define INPUT_BUFFER_SIZE     1 << 5
#define REDRAW_METHOD_RESIZE  (char*) 0
#define REDRAW_METHOD_SRMCUP  (char*) 1
struct context_t {
  int         program_fd;
  pid_t       program_pid;

  uint32_t    mask          :  1; //  1
  uint32_t    stop_output   :  1; //  2 (unused)
  uint32_t    keymodes_size : 10; // 12
  uint32_t    repeat        :  8; // 20
  uint32_t    rehandeled    :  4; // 24
  uint32_t    input_len     :  5; // 29 -> overflow on 32 to 0
  uint32_t    stack_index   :  3; // 32 -> overflow on  8 to 0

  char        input_buffer[INPUT_BUFFER_SIZE];
  keymode_t   global_mode;
  keymode_t   default_mode;
  keymode_t **keymodes;
  keymode_t  *modestack[MODESTACK_SIZE];
  keymode_t  *current_mode;
  binding_t  *current_binding;
  char       *redraw_method;

  struct termios tios_restore;
};

void context_init();
void context_free();
keymode_t* get_keymode(const char *);
keymode_t* add_keymode(const char *);

int   handle_key(TermKeyKey*);
void  writes_to_program(const char *);
void  writeb_to_program(const char *, ssize_t);
int   check_args(int argc, const char *args); 
int   start_program_output();
void  stop_program_output();
const char *key_parse_get_code(const char *);

int   forkapp(char **, int*, pid_t*);
void  set_input_mode();
void  update_pty_size(int);
void  set_cursor(int fd, int x, int y);
void  get_cursor(int fd, int *x, int *y);
void  refresh_window();

void  redraw_begin();
void  redraw_redraw();

#endif
