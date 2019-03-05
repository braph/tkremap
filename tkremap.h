#ifndef _TKREMAP_H
#define _TKREMAP_H

#include <errno.h>
#include <stdint.h>
#include <termios.h>
#include <pthread.h>
#include <termkey.h>
#include <sys/types.h>

#include "options.h"
#include "errormsg.h"
#include "termkeystuff.h"

struct context_t;
struct binding_t;
struct command_call_t;

// === Global variables ===
extern struct context_t context;
// ========================

// === Command stuff ==========================================================
typedef const struct command_opt_t {
   const char  opt;
   const char *meta;
   const char *desc;
} command_opt_t;

#define COMMAND_T_INIT \
   .args = NULL, .opts = NULL, \
   .call = NULL, .free = NULL, .parse = NULL
typedef struct command_t {
   const char           *name;
   const char           *desc;
   const char          **args;
   const command_opt_t  *opts;
   void*               (*parse) (int, char **, option*);
   int                 (*call)  (struct command_call_t*, TermKeyKey*);
   void                (*free)  (void*);
} command_t;

typedef struct command_call_t {
   command_t  *command;
   void       *arg;
} command_call_t;

#define COMMAND_CALL_FUNC(NAME)  int   NAME(struct command_call_t* cmd, TermKeyKey *key)
#define COMMAND_PARSE_FUNC(NAME) void* NAME(int argc, char *args[], option *options)

// === Bindings ===============================================================
#define BINDING_TYPE_COMMAND 0
#define BINDING_TYPE_CHAINED 1
typedef struct binding_t {
   uint16_t  type :  1;
   uint16_t  size : 15;
   union {
      struct command_call_t *commands;
      struct binding_t     **bindings;
   } p;

   TermKeyKey  key; // at the bottom, important!
} binding_t;

typedef struct binding_root_t {
   uint16_t  type :  1;
   uint16_t  size : 15;
   union {
      struct command_call_t *commands;
      struct binding_t     **bindings;
   } p;
} binding_root_t;

void       binding_free(binding_t *);
binding_t* binding_get_binding(binding_t *, TermKeyKey *);
binding_t* binding_add_binding(binding_t *, binding_t *);
void       binding_del_binding(binding_t *, TermKeyKey*);
// ============================================================================


typedef struct keymode_t {
   char       *name;
   uint8_t     repeat_enabled;
   binding_t  *unbound[4];
   binding_t  *root;
} keymode_t;

void  keymode_init(keymode_t*, const char*);
void  keymode_free(keymode_t*);

#define REHANDLE_DEPTH_MAX 10
#define MODESTACK_SIZE 1 << 3
struct context_t {
  int         program_fd;
  pid_t       program_pid;

  uint32_t    mask        :  1; // 1
  uint32_t    stop_output :  1; // 2
  uint32_t    n_keymodes  : 10; //12
  uint32_t    repeat      :  8; //20
  uint32_t    rehandeled  :  4; //24
  uint32_t    input_len   :  5; //29 -> overflow on 32 to 0
  uint32_t    stack_index :  3; //32 -> overflow on  8 to 0

  char        input_buffer[1 << 5];
  keymode_t   global_mode;
  keymode_t   default_mode;
  keymode_t **keymodes;
  keymode_t  *modestack[MODESTACK_SIZE];
  keymode_t  *current_mode;
  binding_t  *current_binding;

  struct termios tios_restore;
  pthread_t      redir_thread;
};

void context_init();
void context_free();
keymode_t* get_keymode(const char *name);
keymode_t* add_keymode(const char *name);

void  handle_key(TermKeyKey*);
void  writes_to_program(const char *);
void  writeb_to_program(const char *, ssize_t);
int   check_args(int argc, const char *args[]); 
char* args_get_arg(int *, char***, const char*);
int   start_program_output();
void  stop_program_output();
const char *key_parse_get_code(const char *);

void  set_input_mode();
void  get_cursor(int fd, int *x, int *y);
void  set_cursor(int fd, int x, int y);
void  update_pty_size(int);
int   forkapp(char **, int *, pid_t *);

typedef struct command_args_t {
   int         argc;
   char      **args;
} command_args_t;

void* copyargs(int, char *[], option*);
void  unpackargs(int *, char ***, option**, command_args_t*);
void  deleteargs(void *);

#endif
