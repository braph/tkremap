#include "tkremap.h"

#include <string.h>
#include <signal.h>

// $ kill -l | grep -Eo '[A-Z12]{3,}' |sort| sed -r 's/.*/{ SIG&,\t "&"\t }/g'

static const struct __packed {
  uint8_t number;
  char    name[6];
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
#define SIGNAL_SIZE (sizeof(signals) / sizeof(signals[0]))

#define STRCASEQ_SIG(S) \
  ((S[0]=='s'||S[0]=='S') && (S[1]=='i'||S[1]=='I') && (S[2]=='g'||S[2]=='G'))

static int name2signal(const char *name) {
  if (STRCASEQ_SIG(name))
    name += 3;

  for (int i = SIGNAL_SIZE; i--;)
    if (! strcasecmp(name, signals[i].name))
      return signals[i].number;

  return 0;
}

#if 0
static char* signal2name(int number) {
  for (int i = 0; i < SIGNAL_SIZE; ++i)
    if (signals[i].number == number)
      return signals[i].name;

  return NULL;
}
#endif

static COMMAND_PARSE_FUNC(parse) {
  #define number argc

  if ((number = atoi(args[0])) > 0)
    return (void*) (uintptr_t) number;

  if (! (number = name2signal(args[0])))
    error_set(EINVAL, args[0]);

  return (void*) (uintptr_t) number;
}

static COMMAND_CALL_FUNC(call) {
  return ! kill(context.program_pid, (int) (uintptr_t) cmd->arg);
}

const command_t command_signal = {
  .name  = "signal"
    "\0Send signal to program",
  .args  = "SIGNAME|NUM\0",
  .parse = &parse,
  .call  = &call
};
