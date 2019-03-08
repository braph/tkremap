#include "termkeystuff.h"
#include "errormsg.h"
#include "tkremap.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <term.h>
#include <curses.h>

TermKey *tk;

typedef struct lookup_t {
  TermKeySym   sym;
  const char   *sequence;
} lookup_t;

static lookup_t *normal_lookup;
static int       normal_lookup_size;

static lookup_t *shift_lookup;
static int       shift_lookup_size;

// Stolen and adapted from libtermkey/driver-ti.c 
static struct {
  const char *funcname;
  TermKeySym sym;
} funcs[] = {
  { "backspace", TERMKEY_SYM_BACKSPACE },
  { "begin",     TERMKEY_SYM_BEGIN     },
  { "beg",       TERMKEY_SYM_BEGIN     },
//{ "btab",      TERMKEY_SYM_TAB,         TERMKEY_KEYMOD_SHIFT }
//{ "cancel",    TERMKEY_SYM_CANCEL    },
//{ "clear",     TERMKEY_SYM_CLEAR     },
//{ "close",     TERMKEY_SYM_CLOSE     },
//{ "command",   TERMKEY_SYM_COMMAND   },
//{ "copy",      TERMKEY_SYM_COPY      },
  { "dc",        TERMKEY_SYM_DELETE    },
  { "down",      TERMKEY_SYM_DOWN      },
  { "end",       TERMKEY_SYM_END       },
  { "enter",     TERMKEY_SYM_ENTER     },
//{ "exit",      TERMKEY_SYM_EXIT      },
//{ "find",      TERMKEY_SYM_FIND      },
//{ "help",      TERMKEY_SYM_HELP      },
  { "home",      TERMKEY_SYM_HOME      },
  { "ic",        TERMKEY_SYM_INSERT    },
  { "left",      TERMKEY_SYM_LEFT      },
//{ "mark",      TERMKEY_SYM_MARK      },
//{ "message",   TERMKEY_SYM_MESSAGE   },
//{ "mouse",     TERMKEY_SYM_NONE      },
//{ "move",      TERMKEY_SYM_MOVE      },
  { "next",      TERMKEY_SYM_PAGEDOWN  },
  { "npage",     TERMKEY_SYM_PAGEDOWN  },
//{ "open",      TERMKEY_SYM_OPEN      },
//{ "options",   TERMKEY_SYM_OPTIONS   },
  { "ppage",     TERMKEY_SYM_PAGEUP    },
  { "previous",  TERMKEY_SYM_PAGEUP    },
  { "print",     TERMKEY_SYM_PRINT     },
//{ "redo",      TERMKEY_SYM_REDO      },
//{ "reference", TERMKEY_SYM_REFERENCE },
//{ "refresh",   TERMKEY_SYM_REFRESH   },
//{ "replace",   TERMKEY_SYM_REPLACE   },
//{ "restart",   TERMKEY_SYM_RESTART   },
//{ "resume",    TERMKEY_SYM_RESUME    },
  { "right",     TERMKEY_SYM_RIGHT     },
//{ "save",      TERMKEY_SYM_SAVE      },
//{ "select",    TERMKEY_SYM_SELECT    },
//{ "suspend",   TERMKEY_SYM_SUSPEND   },
//{ "undo",      TERMKEY_SYM_UNDO      },
  { "up",        TERMKEY_SYM_UP        }
//{ NULL }
};

// Stolen and adapted from libtermkey/driver-ti.c 
  static
int funcname2keysym(const char *funcname, TermKeySym *symp)
{
  int start = 0;
  int end   = sizeof(funcs)/sizeof(funcs[0]);

  while (1) {
    int i   = (start+end) / 2;
    int cmp = strcmp(funcname, funcs[i].funcname);

    if (cmp == 0) {
      *symp = funcs[i].sym;
      return 1;
    }
    else if (end == start + 1)
      // That was our last choice and it wasn't it - not found
      break;
    else if (cmp > 0)
      start = i;
    else
      end = i;
  }

  return 0;
}

static
const char* get_sequence_for_sym(lookup_t *table, int size, TermKeySym sym) {
  while (size--)
    if (table[size].sym == sym)
      return table[size].sequence;

  return NULL;
}

static 
void table_add_key_sym(lookup_t *table, int *size, TermKeySym sym, const char *sequence) {
  table[*size].sym      = sym;
  table[*size].sequence = sequence;
  ++(*size);
}

// Stolen and adapted from libtermkey/driver-ti.c 
int load_terminfo()
{
  int i;
  TermKeySym sym;
  normal_lookup = malloc(sizeof(funcs)/sizeof(funcs[0]) * sizeof(lookup_t));
  normal_lookup_size = 0;
  shift_lookup  = malloc(sizeof(funcs)/sizeof(funcs[0]) * sizeof(lookup_t));
  shift_lookup_size = 0;

  if (setupterm(NULL, 1, NULL) != OK)
    return 0;

  for (i = 0; strfnames[i]; i++)
  {
    // Only care about the key_* constants
    const char *name = strfnames[i];
    if (strncmp(name, "key_", 4) != 0)
      continue;
    if (strcmp(name + 4, "mouse") == 0)
      continue;

    if(funcname2keysym(name + 4, &sym)) {
      if(sym == TERMKEY_SYM_NONE)
        continue;

      const char *value = tigetstr(strnames[i]);
      if (value == 0)
        continue;

      normal_lookup[normal_lookup_size].sym = sym;
      normal_lookup[normal_lookup_size].sequence = value;
      ++normal_lookup_size;
    }
    else if (funcname2keysym(name + 5, &sym)) {
      if(sym == TERMKEY_SYM_NONE)
        continue;

      const char *value = tigetstr(strnames[i]);
      if (value == 0)
        continue;

      shift_lookup[shift_lookup_size].sym = sym;
      shift_lookup[shift_lookup_size].sequence = value;
      ++shift_lookup_size;
    }
  }

  table_add_key_sym(normal_lookup, &normal_lookup_size, TERMKEY_SYM_ENTER,  "\x0D");
  table_add_key_sym(normal_lookup, &normal_lookup_size, TERMKEY_SYM_DEL,    "\x7F");
  table_add_key_sym(normal_lookup, &normal_lookup_size, TERMKEY_SYM_ESCAPE, "\x1B");
  // CTRL+Space

  normal_lookup = realloc(normal_lookup, normal_lookup_size * sizeof(lookup_t));
  shift_lookup  = realloc(shift_lookup,  shift_lookup_size * sizeof(lookup_t));

  return 1;
}

#if FREE_MEMORY
void unload_terminfo() {
  del_curterm(cur_term);
  free(normal_lookup);
  free(shift_lookup);
}
#endif

#if 0
TermKeyKey* parse_key_new(const char *def) {
  TermKeyKey *key = malloc(sizeof(*key));

  if (! parse_key(def, key))
    return free(key), NULL;

  return key;
}
#endif

int parse_key(const char *def, TermKeyKey *key) {
  const char *last_char;

  // Try default
  last_char = termkey_strpkey(tk, def, key, 0);
  if (last_char != NULL && *last_char == 0)
    return 1;

  // Try Meta-k, M-k instead of Alt-k, A-k
  last_char = termkey_strpkey(tk, def, key, TERMKEY_FORMAT_ALTISMETA);
  if (last_char != NULL && *last_char == 0)
    return 1;

  // Try Shift-k, Control-k instead of S-k, C-k
  last_char = termkey_strpkey(tk, def, key, TERMKEY_FORMAT_LONGMOD);
  if (last_char != NULL && *last_char == 0)
    return 1;

  // Try ^K
  last_char = termkey_strpkey(tk, def, key, TERMKEY_FORMAT_CARETCTRL);
  if (last_char != NULL && *last_char == 0)
    return 1;

  // Try ^k (lower char)
  if (def[0] == '^' && def[1] >= 'a' && def[1] <= 'z' && def[2] == 0) {
    char def2[3] = { '^', 0, 0 };
    def2[1] = toupper(def[1]);
    last_char = termkey_strpkey(tk, def2, key, TERMKEY_FORMAT_CARETCTRL);
    if (last_char != NULL && *last_char == 0)
      return 1;
  }

  return error_write("%s: %s", E_INVALID_KEY, def), 0;
}

#if DEBUG
const char *format_key(TermKeyKey *key) {
  static char buf[32];
  termkey_strfkey(tk, buf, sizeof(buf), key, 0);
  return buf;
}
#endif

static inline __attribute__((always_inline))
  char get_byte_for_mod(int modifiers) {
    /*  For KEYSYM and Fn-keys
     *    base        = 49
     *    Shift       =  1
     *    Alt         =  2
     *    Cntrl       =  4  */

    return 49
      + (1 * (!! (modifiers & TERMKEY_KEYMOD_SHIFT)))
      + (2 * (!! (modifiers & TERMKEY_KEYMOD_ALT)))
      + (4 * (!! (modifiers & TERMKEY_KEYMOD_CTRL)));
  }

const char *get_key_code(TermKeyKey *key) {
  static char buf[8];
  char mod, c;

  if (key->type == TERMKEY_TYPE_UNICODE) {
    c = key->code.codepoint;

    if (key->modifiers & TERMKEY_KEYMOD_CTRL)
      c -= 0x60;

    if (key->modifiers & TERMKEY_KEYMOD_ALT) {
      buf[0] = 27;
      buf[1] = c;
      buf[2] = 0;
    }
    else {
      buf[0] = c;
      buf[1] = 0;
    }
    return buf;
  }
  else if (key->type == TERMKEY_TYPE_KEYSYM) {
    if (key->modifiers == 0)
      return get_sequence_for_sym(normal_lookup, normal_lookup_size, key->code.sym);

    #define case break; case
    switch ((int) key->code.sym) {
      case TERMKEY_SYM_INSERT:    c = '2';
      case TERMKEY_SYM_DELETE:    c = '3';
      case TERMKEY_SYM_PAGEUP:    c = '5';
      case TERMKEY_SYM_PAGEDOWN:  c = '6';

      case TERMKEY_SYM_UP:        c = 'A';
      case TERMKEY_SYM_DOWN:      c = 'B';
      case TERMKEY_SYM_RIGHT:     c = 'C';
      case TERMKEY_SYM_LEFT:      c = 'D';
      case TERMKEY_SYM_END:       c = 'F';
      case TERMKEY_SYM_HOME:      c = 'H';
      break; default: return NULL;
    }
    #undef case

    mod = get_byte_for_mod(key->modifiers);
    buf[0] = 27;
    buf[1] = 91;
    buf[2] = (c <= '6' ? c   : 49);
    buf[3] = 59;
    buf[4] = mod;
    buf[5] = (c <= '6' ? 126 : c);
    buf[6] = 0;
    return buf;
  }
  else if (key->type == TERMKEY_TYPE_FUNCTION) {
    int n = key->code.number;
    if (n > 12)
      return NULL;

    buf[0] = 27;
    if (key->modifiers == 0) {
      #if ______B0___B1___B2___B3___B4___B5
        F1-F4:  27   79   79+N 0    0    0
       F5-F12:  27   91   X1   X2   126  0
      #endif

      if (n <= 4) { // F1-F4
        buf[1] = 79;
        buf[2] = 79 + n;
        buf[3] = 0;
      }
      else {        // F5-F12
        buf[1] = 91;
        buf[2] = (n <= 8 ? 49 : 50); // X1
        buf[3] = (char[]){ 53, 55, 56, 57,48, 49, 51, 52 }[n - 5]; // X2
        buf[4] = 126;
        buf[5] = 0;
      }

      return buf;
    }
    else {
      #if __________B0___B1___B2___B3___B4___B5___B6
       (M)  F1-F4:  27   91   X1   59   MOD  79+N 0
       (M) F5-F12:  27   91   X1   X2   59   MOD  126
      #endif

      mod = get_byte_for_mod(key->modifiers);
      buf[1] = 91;
      buf[2] = (n <= 8 ? 49 : 50); // X1
      buf[3] = (char[]) { 59, 59, 59, 59, 53, 55, 56, 57, 48, 49, 51, 52 } [n - 1]; // X2

      if (n <= 4) { // F1 - F4
        buf[4] = mod;
        buf[5] = 79 + n;
        buf[6] = 0;
      }
      else {        // F5 - F12
        buf[4] = 59;
        buf[5] = mod;
        buf[6] = 126;
        buf[7] = 0;
      }
      return buf;
    }
  }

  return NULL;
}
