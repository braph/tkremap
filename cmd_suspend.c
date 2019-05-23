#include "tkremap.h"
#include "common.h"

/* Maybe use get_cursor() and set_cursor()?
 *  int x,y;
 *  get_cursor(STDIN_FILENO, &y, &x);
 *  set_cursor(STDOUT_FILENO, y, x);
 */

static COMMAND_CALL_FUNC(call) {
  #define PID context.program_pid
  kill(PID, SIGSTOP); // Stop program
  redraw_begin();     // Save screen
  raise(SIGTSTP);     // Stop self
  kill(PID, SIGCONT); // Continue program
  set_input_mode();   // Restore input mode
  redraw_redraw();    // Restore screen
  return 1;
}

const command_t command_suspend = {
  .name = "suspend (^Z)"
    "\0Suspend tkremap together with the program (see also *redraw\\_method*)",
  .call = &call
};

