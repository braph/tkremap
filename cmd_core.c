#include "tkremap.h"
#include "common.h"

// === mask ===================================================================
static COMMAND_CALL_FUNC(cmd_mask) {
  return (context.mask = 1);
}

const command_t command_mask = {
  COMMAND_T_INIT,
  .name  = "mask",
  .desc  = "Do not interprete the next keypress as a keybinding",
  .call  = &cmd_mask,
};

// === pass ===================================================================
static COMMAND_CALL_FUNC(cmd_pass) {
  return writeb_to_program(context.input_buffer, context.input_len), 1;
}

const command_t command_pass = {
  COMMAND_T_INIT,
  .name  = "pass",
  .desc  = "Send the pressed key to the program",
  .call  = &cmd_pass,
};

// === ignore =================================================================
static COMMAND_CALL_FUNC(cmd_ignore) {
  return 1;
}

const command_t command_ignore = {
  COMMAND_T_INIT,
  .name  = "ignore",
  .desc  = "Completely ignore the current pressed key",
  .call  = &cmd_ignore,
};

// === mode ===================================================================
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
  if (streq(args[0], "-p"))
    return (void*) (uintptr_t) 1;

  keymode_t *km = get_keymode(args[0]);

  if (! km)
    return add_keymode(args[0]);

  return (void*) km;
}

const command_t command_mode = {
  COMMAND_T_INIT,
  .name  = "mode",
  .desc  = "Switch to _MODE_\n\nPass *-p* for previous mode",
  .args  = (const char*[]) { "MODE|-p", 0 },
  .call  = &cmd_mode,
  .parse = &cmd_mode_parse
};

// === repeat =================================================================
#define REPEAT_ON   2
#define REPEAT_OFF  1

static COMMAND_CALL_FUNC(cmd_repeat) {
  context.current_mode->repeat_enabled = ((int) (uintptr_t) cmd->arg) - 1;
  return 1;
}

static COMMAND_PARSE_FUNC(cmd_repeat_parse) {
  if (streq(args[0], "on"))
    return (void*) (uintptr_t) REPEAT_ON;
  else if (streq(args[0], "off"))
    return (void*) (uintptr_t) REPEAT_OFF;
  else
    return error_write("%s: '%s' {on|off}", strerror(EINVAL), args[0]), NULL;
}

command_t command_repeat = {
  COMMAND_T_INIT,
  .name  = "repeat",
  .desc  = "Enable repetition mode"
    ,.args  = (const char*[]) { "on|off", 0 },
  .call  = &cmd_repeat,
  .parse = &cmd_repeat_parse
};

// === rehandle ===============================================================
static COMMAND_CALL_FUNC(cmd_rehandle) {
  if (context.rehandeled < REHANDLE_DEPTH_MAX)
    ++context.rehandeled,
      handle_key(key);
  else
    context.rehandeled = 0;
  return 1;
}

const command_t command_rehandle = {
  COMMAND_T_INIT,
  .name  = "rehandle",
  .desc  = "Rehandle the current key",
  .call  = &cmd_rehandle,
};

