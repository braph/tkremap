#include "termkeystuff.h"
#include "tkremap.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <curses.h>
#include <term.h>

TermKey *tk;

typedef struct __packed lookup_t {
  char          *sequence;
  TermKeySym     sym;
} lookup_t;

static lookup_t *normal_lookup;
static int       normal_lookup_size;

static lookup_t *shift_lookup;
static int       shift_lookup_size;

// Stolen and adapted from libtermkey/driver-ti.c 
static const struct __packed {
  char *funcname;
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
#define FUNCS_SIZE (sizeof(funcs) / sizeof(funcs[0]))

// Stolen and adapted from libtermkey/driver-ti.c 
static int funcname2keysym(const char *funcname, TermKeySym *symp)
{
  int start = 0;
  int end   = FUNCS_SIZE;

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
  table[*size].sequence = strdup(sequence);
  ++(*size);
}

// Stolen and adapted from libtermkey/driver-ti.c 
int load_terminfo()
{
  int i;
  TermKeySym sym;
  lookup_t normal[FUNCS_SIZE + 5]; // Will be copied
  lookup_t  shift[FUNCS_SIZE + 5]; // to heap later

  normal_lookup = normal;
  shift_lookup  = shift;
  normal_lookup_size = 0;
  shift_lookup_size  = 0;

  if (setupterm(NULL, 1, NULL) != OK)
    return 0;

  for (i = 0; strfnames[i]; i++)
  {
    // Only care about the key_* constants
    const char *name = strfnames[i];
    if (name[0] != 'k' || name[1] != 'e' || name[2] != 'y' || name[3] != '_')
      continue;
    if (strcmp(name + 4, "mouse") == 0)
      continue;

    if (funcname2keysym(name + 4, &sym)) {
      if (sym == TERMKEY_SYM_NONE)
        continue;

      const char *value = tigetstr(strnames[i]);
      if (value == 0)
        continue;

      table_add_key_sym(normal_lookup, &normal_lookup_size, sym, value);
      //normal_lookup[normal_lookup_size].sym = sym;
      //normal_lookup[normal_lookup_size].sequence = strdup(value);
      //++normal_lookup_size;
    }
    else if (funcname2keysym(name + 5, &sym)) {
      if (sym == TERMKEY_SYM_NONE)
        continue;

      const char *value = tigetstr(strnames[i]);
      if (value == 0)
        continue;

      table_add_key_sym(shift_lookup, &shift_lookup_size, sym, value);
      //shift_lookup[shift_lookup_size].sym = sym;
      //shift_lookup[shift_lookup_size].sequence = strdup(value);
      //++shift_lookup_size;
    }
  }

  table_add_key_sym(normal_lookup, &normal_lookup_size, TERMKEY_SYM_ENTER,  "\x0D");
  table_add_key_sym(normal_lookup, &normal_lookup_size, TERMKEY_SYM_DEL,    "\x7F");
  table_add_key_sym(normal_lookup, &normal_lookup_size, TERMKEY_SYM_ESCAPE, "\x1B");
  // TODO: Add CTRL+Space

  normal_lookup = memdup(normal_lookup, normal_lookup_size * sizeof(lookup_t));
  shift_lookup  = memdup(shift_lookup,  shift_lookup_size  * sizeof(lookup_t));

  del_curterm(cur_term);
  return 1;
}

#define TERMKEY_STRPKEY(STR, KEY, FLAGS) \
  ((last_char = termkey_strpkey(tk, STR, KEY, FLAGS)) != NULL && *last_char==0)

#define TOUPPER_SIMPLE(CH) \
  (CH - 32)

#define TOLOWER_SIMPLE(CH) \
  (CH + 32)

#define ISLOWER_ASCII(CH) \
  (CH >= 'a' && CH <= 'z')

#define ISUPPER_ASCII(CH) \
  (CH >= 'A' && CH <= 'Z')

int parse_key(const char *def, TermKeyKey *key) {
  const char *last_char;
  const int fmts[] = {
    0, // Default

    // Try A-k instead of M-k
    TERMKEY_FORMAT_ALTISMETA,                       // A-
    TERMKEY_FORMAT_ALTISMETA|TERMKEY_FORMAT_LONGMOD,// Meta, Alt
    TERMKEY_FORMAT_LONGMOD,                         // Shift-, Control-, Meta-
    TERMKEY_FORMAT_LONGMOD|TERMKEY_FORMAT_LOWERMOD, // shift-, control-, 

    // Try ^K
    TERMKEY_FORMAT_CARETCTRL
  };

  for (int i = -1; ++i < sizeof(fmts)/sizeof(fmts[0]);)
    if (TERMKEY_STRPKEY(def, key, fmts[i]))
      goto RETURN;

  /*
  // Try default
  if (TERMKEY_STRPKEY(def, key, 0))
    goto RETURN;

  // Try A-k instead of M-k
  if (TERMKEY_STRPKEY(def, key, TERMKEY_FORMAT_ALTISMETA))
    goto RETURN;

  // Try Meta-k, Alt-k instead of M-k, A-k
  if (TERMKEY_STRPKEY(def, key, TERMKEY_FORMAT_ALTISMETA|TERMKEY_FORMAT_LONGMOD))
    goto RETURN;

  // Try Shift-k, Control-k instead of S-k, C-k
  if (TERMKEY_STRPKEY(def, key, TERMKEY_FORMAT_LONGMOD))
    goto RETURN;

  // Try shift-k, control-k instead of Shift-k, Ctrl-k
  if (TERMKEY_STRPKEY(def, key, TERMKEY_FORMAT_LONGMOD|TERMKEY_FORMAT_LOWERMOD))
    goto RETURN;

  // Try ^K
  if (TERMKEY_STRPKEY(def, key, TERMKEY_FORMAT_CARETCTRL))
    goto RETURN;
  */

  // Try ^k (make lower char to upper)
  if (def[0] == '^' && ISLOWER_ASCII(def[1]) && def[2] == 0) {
    char def_upper[3] = { '^', TOUPPER_SIMPLE(def[1]), 0 };
    if (TERMKEY_STRPKEY(def_upper, key, TERMKEY_FORMAT_CARETCTRL))
      goto RETURN;
  }

  error_set_errno(E_INVALID_KEY);
  return 0;

RETURN:
  /* FIX:
   * When parsing 
   *  `Shift-<LOWER>` (or `Meta-Shift-<LOWER>` etc.)
   * key.code.codepoint will be left lowercase.
   * This will confuse `get_key_code()`, causing an invalid keycode to be returned.
   */

  if (key->modifiers & TERMKEY_KEYMOD_SHIFT &&
      ISLOWER_ASCII(key->code.codepoint))
    key->code.codepoint = TOUPPER_SIMPLE(key->code.codepoint);

  /* FIX:
   * When parsing
   *  `Ctrl + <Upper Char>` or `Ctrl + Shift + <Char>`
   * key.code.codepoint will be uppercase.
   * This will confuse `get_key_code()`, causing an invalid keycode to be returned.
   * (There are only uppercase Ctrl combinations)
   */

  if (key->modifiers & TERMKEY_KEYMOD_CTRL &&
      ISUPPER_ASCII(key->code.codepoint))
    key->code.codepoint = TOLOWER_SIMPLE(key->code.codepoint);

  return 1;
}

static char __inline get_byte_for_mod(int modifiers) {
  /*  For KEYSYM and Function-keys
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

  error_set_errno(E_KEYCODE_NA);
  return NULL;
}

#if DEBUG
const char *format_key(TermKeyKey *key) {
  static char buf[32];
  termkey_strfkey(tk, buf, sizeof(buf), key, 0);
  return buf;
}
#endif

#if FREE_MEMORY
void unload_terminfo() {
  ___("unload_terminfo");

  while (normal_lookup_size--)
    free(normal_lookup[normal_lookup_size].sequence);

  while (shift_lookup_size--)
    free(shift_lookup[shift_lookup_size].sequence);

  free(normal_lookup);
  free(shift_lookup);
}
#endif

