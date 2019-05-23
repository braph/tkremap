// vim pattern for swapping fields:
//  s/\v\{ *("[a-z]+") *, * ([A-Z_]+) * \}/{ \2,              \1 }/g

//version 1:
  // Try default
  last_char = termkey_strpkey(tk, def, key, 0);
  if (last_char != NULL && *last_char == 0)
    return 1;

  // Try A-k instead of M-k
  last_char = termkey_strpkey(tk, def, key, TERMKEY_FORMAT_ALTISMETA);
  if (last_char != NULL && *last_char == 0)
    return 1;

  // Try Meta-k, Alt-k instead of M-k, A-k
  last_char = termkey_strpkey(tk, def, key, TERMKEY_FORMAT_ALTISMETA|TERMKEY_FORMAT_LONGMOD);
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


// version 2:
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






static struct __packed {
  TermKeySym sym;
  char funcname[10];
  //char *funcname;
} funcs[] = {
  { TERMKEY_SYM_BACKSPACE, "backspace" },
  { TERMKEY_SYM_BEGIN,     "begin"     },
  { TERMKEY_SYM_BEGIN,     "beg"       },
//{ TERMKEY_SYM_TAB,       "btab"      }, TERMKEY_KEYMOD_SHIFT }
//{ TERMKEY_SYM_CANCEL,    "cancel"    },
//{ TERMKEY_SYM_CLEAR,     "clear"     },
//{ TERMKEY_SYM_CLOSE,     "close"     },
//{ TERMKEY_SYM_COMMAND,   "command"   },
//{ TERMKEY_SYM_COPY,      "copy"      },
  { TERMKEY_SYM_DELETE,    "dc"        },
  { TERMKEY_SYM_DOWN,      "down"      },
  { TERMKEY_SYM_END,       "end"       },
  { TERMKEY_SYM_ENTER,     "enter"     },
//{ TERMKEY_SYM_EXIT,      "exit"      },
//{ TERMKEY_SYM_FIND,      "find"      },
//{ TERMKEY_SYM_HELP,      "help"      },
  { TERMKEY_SYM_HOME,      "home"      },
  { TERMKEY_SYM_INSERT,    "ic"        },
  { TERMKEY_SYM_LEFT,      "left"      },
//{ TERMKEY_SYM_MARK,      "mark"      },
//{ TERMKEY_SYM_MESSAGE,   "message"   },
//{ TERMKEY_SYM_NONE,      "mouse"     },
//{ TERMKEY_SYM_MOVE,      "move"      },
  { TERMKEY_SYM_PAGEDOWN,  "next"      },
  { TERMKEY_SYM_PAGEDOWN,  "npage"     },
//{ TERMKEY_SYM_OPEN,      "open"      },
//{ TERMKEY_SYM_OPTIONS,   "options"   },
  { TERMKEY_SYM_PAGEUP,    "ppage"     },
  { TERMKEY_SYM_PAGEUP,    "previous"  },
  { TERMKEY_SYM_PRINT,     "print"     },
//{ TERMKEY_SYM_REDO,      "redo"      },
//{ TERMKEY_SYM_REFERENCE, "reference" },
//{ TERMKEY_SYM_REFRESH,   "refresh"   },
//{ TERMKEY_SYM_REPLACE,   "replace"   },
//{ TERMKEY_SYM_RESTART,   "restart"   },
//{ TERMKEY_SYM_RESUME,    "resume"    },
  { TERMKEY_SYM_RIGHT,     "right"     },
//{ TERMKEY_SYM_SAVE,      "save"      },
//{ TERMKEY_SYM_SELECT,    "select"    },
//{ TERMKEY_SYM_SUSPEND,   "suspend"   },
//{ TERMKEY_SYM_UNDO,      "undo"      },
  { TERMKEY_SYM_UP,        "up"        }
//{ NULL }
};
