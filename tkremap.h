#ifndef _TKREMAP_H
#define _TKREMAP_H

#include <errno.h>
#include <stdint.h>
#include <termios.h>
#include <pthread.h>
#include <termkey.h>
#include <sys/types.h>

#include "options.h"
#include "termkeystuff.h"

struct context_t;
struct binding_t;
struct command_call_t;

// === Global variables ===
extern struct context_t context;
// ========================

char* get_error();
void  write_error(const char *fmt, ...);
void  prepend_error(const char *fmt, ...);

// === Command stuff ==========================================================
typedef const struct command_opt_t {
   const char  opt;
   const char *meta;
   const char *desc;
} command_opt_t;

#define COMMAND_T_INIT \
   .args  = NULL, .opts = NULL, \
   .parse = NULL, .free = NULL, .call = NULL
typedef struct command_t {
   const char           *name;
   const char           *desc;
   const char          **args;
   const command_opt_t  *opts;
   void*               (*parse) (int, char **, option*);
   void                (*call)  (struct command_call_t*, TermKeyKey*);
   void                (*free)  (void*);
} command_t;

typedef struct command_call_t {
   command_t  *command;
   void       *arg;
} command_call_t;

typedef struct command_args_t {
   int         argc;
   char      **args;
} command_args_t;

#define COMMAND_CALL_FUNC(NAME)  void  NAME (struct command_call_t* cmd, TermKeyKey *key)
#define COMMAND_PARSE_FUNC(NAME) void* NAME (int argc, char *args[], option *options)

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


#define UNBOUND_IGNORE 0
#define UNBOUND_PASS   1
#define UNBOUND_REEVAL 2
typedef struct keymode_t {
   char        *name;
   uint16_t    repeat_enabled    : 1;
   uint16_t    unbound_unicode   : 2;
   uint16_t    unbound_keysym    : 2;
   uint16_t    unbound_function  : 2;
   uint16_t    unbound_mouse     : 2;
   binding_t   *root;
} keymode_t;

void  keymode_init(keymode_t*, const char*, int);
void  keymode_free(keymode_t*);


struct context_t {
   int            program_fd;
   pid_t          program_pid;

   uint32_t       mask        :  1;
   uint32_t       stop_output :  1;
   uint32_t       n_keymodes  : 12;
   uint32_t       repeat      : 12;
   uint32_t       input_len   :  6;

   char           *input_buffer;
   keymode_t      global_mode;
   keymode_t      default_mode;
   keymode_t      **keymodes;
   keymode_t      *current_mode;
   binding_t      *current_binding;

   struct termios tios_restore;
   pthread_t      redir_thread;
};

void context_init();
void context_free();
keymode_t* get_keymode(const char *name);
keymode_t* add_keymode(const char *name);

void   handle_key(TermKeyKey*);
void   writes_to_program(const char *);
void   writeb_to_program(const char *, ssize_t);
int    check_args(int argc, const char *args[]); 
char*  args_get_arg(int *, char***, const char*);
int    start_program_output();
void   stop_program_output();
const char *key_parse_get_code(const char *);

char**   argsdup(int argc, char **args);
void*    copyargs(int argc, char *args[], option* options);
void     unpackargs(int *argc, char ***args, option** options, command_args_t*);
void     deleteargs(void *_args);

void  set_input_mode();
void  get_cursor(int fd, int *x, int *y);
void  set_cursor(int fd, int x, int y);
void  update_pty_size(int);
int   forkapp(char **, int *, pid_t *);

#endif
