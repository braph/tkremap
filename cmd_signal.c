#include "tkremap.h"

#include <string.h>
#include <signal.h>

static struct {
  int   number;
  char  *name;
} signals[] = {
  { SIGINT,   "INT"   },
  { SIGHUP,   "HUP"   },
  { SIGTERM,  "TERM"  },
  { SIGKILL,  "KILL"  },
  { SIGTSTP,  "TSTP"  },
  { SIGSTOP,  "STOP"  },
  { SIGCONT,  "CONT"  },
  { SIGUSR1,  "USR1"  },
  { SIGUSR2,  "USR2"  },
  { SIGALRM,  "ALRM"  },
  { SIGWINCH, "WINCH" }
};
#define SIGNAL_SIZE (sizeof(signals)/sizeof(signals[0]))

static
int name2signal(const char *name) {
  if (! strncasecmp(name, "SIG", 3))
    name += 3;

  for (int i = SIGNAL_SIZE; i--; )
    if (! strcasecmp(name, signals[i].name))
      return signals[i].number;

  return 0;
}

#if 0
static
char* signal2name(int number) {
  for (int i = 0; i < SIGNAL_SIZE; ++i)
    if (signals[i].number == number)
      return signals[i].name;

  return NULL;
}
#endif

static COMMAND_CALL_FUNC(call) {
  int sig = (int) (uintptr_t) cmd->arg;
  return !kill(context.program_pid, sig);
}

static COMMAND_PARSE_FUNC(parse) {
  #define number argc

  if (! (number = name2signal(args[0])))
    error_write("%s: %s", strerror(EINVAL), args[0]);

  return (void*) (uintptr_t) number; // is NULL if failed
}

command_t command_signal = {
  .name  = "signal",
  .desc  = "Send signal to program",
  .args  = (const char *[]) { "SIGNAL", 0 },
  .parse = &parse,
  .call  = &call,
};
