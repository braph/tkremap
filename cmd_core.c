#include "tkremap.h"

#define COMMAND_T_INIT \
   .args  = NULL,  .opts  = NULL, \
   .parse = NULL,  .free  = NULL

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
   .desc  = "Do nothing\n"
            "This command can be used to ignore keypresses.",
   .call  = &cmd_ignore,
};

// === goto ===================================================================
static COMMAND_CALL_FUNC(cmd_goto) {
   context.current_mode = (keymode_t*) cmd->arg;
}

static COMMAND_PARSE_FUNC(cmd_goto_parse) {
   keymode_t *km = get_keymode(args[0]);

   if (! km)
      return add_keymode(args[0]);

   return (void*) km;
}

const command_t command_goto = {
   COMMAND_T_INIT,
   .name  = "goto",
   .desc  = "Switch the current mode",
   .args  = (const char*[]) { "MODE", 0 },
   .call  = &cmd_goto,
   .parse = &cmd_goto_parse,
};
