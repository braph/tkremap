#include "tkremap.h"

// === mask ===================================================================
static COMMAND_CALL_FUNC(cmd_mask) {
  context.mask = 1;
  return 1;
}

const command_t command_mask = {
  .name  = "mask"
    "\0Do not interprete the next keypress as a keybinding\n"
    "The key will be passed as is to the program",
  .call  = &cmd_mask,
};

// === pass ===================================================================
static COMMAND_CALL_FUNC(cmd_pass) {
  writeb_to_program(context.input_buffer, context.input_len);
  return 1;
}

const command_t command_pass = {
  .name  = "pass"
    "\0Send the pressed key to the program",
  .call  = &cmd_pass,
};

// === ignore =================================================================
static COMMAND_CALL_FUNC(cmd_ignore) {
  return 1;
}

const command_t command_ignore = {
  .name  = "ignore"
    "\0Completely ignore the current pressed key",
  .call  = &cmd_ignore,
};

// === mode ===================================================================
#define STREQ_OPT_P(S) (S[0]=='-' && S[1]=='p' && S[2]=='\0') // "-p"

static COMMAND_CALL_FUNC(cmd_mode) {
  if (((uintptr_t) cmd->arg) == 1)
    context.current_mode = context.modestack[--context.stack_index];
  else
    context.modestack[++context.stack_index] =
      context.current_mode =
        (keymode_t*) cmd->arg;
  return 1;
}

static COMMAND_PARSE_FUNC(cmd_mode_parse) {
  if (STREQ_OPT_P(args[0]))
    return (void*) (uintptr_t) 1;

  keymode_t *km = get_keymode(args[0]);

  if (! km)
    return add_keymode(args[0]);

  return (void*) km;
}

const command_t command_mode = {
  .name  = "mode"
    "\0Switch to _MODE_\n"
    "\nPass *-p* for previous mode",
  .args  = "MODE|-p\0",
  .call  = &cmd_mode,
  .parse = &cmd_mode_parse
};

// === repeat =================================================================
#define REPEAT_ON     1
#define REPEAT_OFF    0
#define STREQ_ON(S)   (S[0]=='o' && S[1]=='n')
#define STREQ_OFF(S)  (S[0]=='o' && S[1]=='f')

static COMMAND_CALL_FUNC(cmd_repeat) {
  context.current_mode->repeat_enabled = ((int) (uintptr_t) cmd->arg) - 1;
  return 1;
}

static COMMAND_PARSE_FUNC(cmd_repeat_parse) {
  if (STREQ_ON(args[0]))
    return (void*) (uintptr_t) REPEAT_ON + 1;
  else if (STREQ_OFF(args[0]))
    return (void*) (uintptr_t) REPEAT_OFF + 1;
  else
    return error_set_errno(EINVAL), NULL;
}

const command_t command_repeat = {
  .name  = "repeat"
    "\0Enable repetition mode",
  .args  = "on|off\0",
  .call  = &cmd_repeat,
  .parse = &cmd_repeat_parse
};

// === rehandle ===============================================================
static COMMAND_CALL_FUNC(cmd_rehandle) {
  if (key) {
    if (context.rehandeled < REHANDLE_DEPTH_MAX) {
      ++context.rehandeled;
      return handle_key(key);
    }
    else
      return (context.rehandeled = 0);
  }
  return 1;
}

const command_t command_rehandle = {
  .name  = "rehandle"
    "\0Rehandle the current key [in another mode]",
  .call  = &cmd_rehandle
};

// === debug ==================================================================
#if DEBUG
static COMMAND_CALL_FUNC(cmd_debug) {
  write_full(STDERR_FILENO, (char*) cmd->arg, strlen((char*) cmd->arg));
  return 1;
}

static COMMAND_PARSE_FUNC(cmd_debug_parse) {
  return (void*) strdup(args[0]);
}

const command_t command_debug = {
  .name  = "debug",
  .args  = "STRING\0",
  .parse = &cmd_debug_parse,
  .call  = &cmd_debug,
  .free  = &free
};
#endif

