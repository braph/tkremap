#include "tkremap.h"
#include "common.h"

// === mask ===================================================================
static COMMAND_CALL_FUNC(cmd_mask) {
   context.mask = 1;
}

const command_t command_mask = {
   COMMAND_T_INIT,
   .name  = "mask",
   .desc  = "Do not interprete the next keypress as a keybinding",
   .call  = &cmd_mask,
};

// === pass ===================================================================
static COMMAND_CALL_FUNC(cmd_pass) {
   writeb_to_program(context.input_buffer, context.input_len);
}

const command_t command_pass = {
   COMMAND_T_INIT,
   .name  = "pass",
   .desc  = "Write the pressed key to the program",
   .call  = &cmd_pass,
};

// === ignore =================================================================
static COMMAND_CALL_FUNC(cmd_ignore) {
   (void) 0;
}

const command_t command_ignore = {
   COMMAND_T_INIT,
   .name  = "ignore",
   .desc  = "Completely ignore the current pressed key",
   .call  = &cmd_ignore,
};

// === mode ===================================================================
static COMMAND_CALL_FUNC(cmd_mode) {
   context.current_mode = (keymode_t*) cmd->arg;
}

static COMMAND_PARSE_FUNC(cmd_mode_parse) {
   keymode_t *km = get_keymode(args[0]);

   if (! km)
      return add_keymode(args[0]);

   return (void*) km;
}

const command_t command_mode = {
   COMMAND_T_INIT,
   .name  = "mode",
   .desc  = "Switch to _MODE_",
   .args  = (const char*[]) { "MODE", 0 },
   .call  = &cmd_mode,
   .parse = &cmd_mode_parse
};

// === repeat =================================================================
#define REPEAT_ON   2
#define REPEAT_OFF  1

static COMMAND_CALL_FUNC(cmd_repeat) {
   context.current_mode->repeat_enabled = ((int) (uintptr_t) cmd->arg) - 1;
}

static COMMAND_PARSE_FUNC(cmd_repeat_parse) {
   if (streq(args[0], "on"))
      return (void*) (uintptr_t) REPEAT_ON;
   else if (streq(args[0], "off"))
      return (void*) (uintptr_t) REPEAT_OFF;
   else {
      write_error("%s: '%s' {on|off}", strerror(EINVAL), args[0]);
      return NULL;
   }
}

command_t command_repeat = {
   COMMAND_T_INIT,
   .name  = "repeat",
   .desc  = "Enable repetition mode",
   .args  = (const char*[]) { "on|off", 0 },
   .call  = &cmd_repeat,
   .parse = &cmd_repeat_parse
};
