#include "tkremap.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define IS_PLUS(BUF) (BUF[0] == '+' && BUF[1] == '\0')
#define MODE     (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH)

typedef struct cmd_exec_args {
  uint8_t   use_shell;
  uint8_t   background;
  char     *in;
  char     *err;
  char     *out;
  char    **args;
} cmd_exec_args;

// TODO: filedescriptor stuff (close etc.)
static COMMAND_CALL_FUNC(cmd_exec_call) {
  cmd_exec_args *cmd_args = (cmd_exec_args*) cmd->arg;
  int pid;

  if ((pid = fork()) == 0) {
    int outfd = -1;
    int errfd = -1;
    int infd  = -1;

    if (cmd_args->out) {
      if (IS_PLUS(cmd_args->out))
        outfd = context.program_fd;
      else
        outfd = open(cmd_args->out, O_WRONLY|O_CREAT, MODE);
    }

    if (cmd_args->err) {
      if (IS_PLUS(cmd_args->err))
        errfd = context.program_fd;
      else
        errfd = open(cmd_args->err, O_WRONLY|O_CREAT, MODE);
    }

    if (cmd_args->in) {
      infd = open(cmd_args->in, O_RDONLY);
    }

    if (errfd >= 0)
      dup2(errfd, STDERR_FILENO);

    if (outfd >= 0)
      dup2(outfd, STDOUT_FILENO);

    if (infd >= 0)
      dup2(infd, STDIN_FILENO);

    if (cmd_args->use_shell)
      return execl("/bin/sh", "sh", "-c", cmd_args->args[0], (char *) 0), 0;
    else
      return execvp(cmd_args->args[0], cmd_args->args), 0;
  }
  else {
    if (! cmd_args->background) {
      int ret;
      return waitpid(pid, &ret, 0), !WEXITSTATUS(ret);
    }
  }

  return 1;
}

static COMMAND_PARSE_FUNC(cmd_exec_parse) {
  cmd_exec_args cmd_args = {
    .use_shell  = 0,
    .background = 0,
    .in         = NULL,
    .out        = NULL,
    .err        = NULL
  };

  for (option *opt = options; opt->opt; ++opt) {
    #define case break; case
    switch (opt->opt) {
      case 's':
        if (argc > 1)
          return error_write("Only one argument allowed with -s"), NULL;
        cmd_args.use_shell = 1;

      case 'o':
        cmd_args.out = strdup(opt->arg);

      case 'e':
        cmd_args.err = strdup(opt->arg);

      case 'i':
        cmd_args.in  = strdup(opt->arg);
    }
    #undef case
  }

  cmd_args.args = malloc((argc + 1) * sizeof(char*));
  cmd_args.args[argc] = NULL;
  for (int i = argc; i--; )
    cmd_args.args[i]  = strdup(args[i]);

  return MEMDUP(&cmd_args);
}

static void cmd_exec_free(void *_arg) {
  cmd_exec_args *cmd_args = (cmd_exec_args*)_arg;
  freeArrayNULL(cmd_args->args);
  free(cmd_args->out);
  free(cmd_args->err);
  free(cmd_args->in);
  free(cmd_args);
}

const command_t command_exec = {
  .name  = "exec",
  .desc  = 
    "Redirect program output to program",
  .args  = (const char*[]) { "COMMAND", "*ARGS", 0 },
  .opts  = (const command_opt_t[]) {
    {'s', NULL,     "Pass _COMMAND_ to /bin/sh instead of invoking it using *exec(3)*"},
    {'b', NULL,     "Run _COMMAND_ in background"},
    {'o', "FILE",   "Redirect STDOUT of _COMMAND_ to _FILE_. Pass `*+*` for redirecting to program's STDIN"},
    {'e', "FILE",   "Redirect STDERR of _COMMAND_ to _FILE_. Pass `*+*` for redirecting to program's STDIN"},
    {'i', "FILE",   "Use _FILE_ as STDIN for _COMMAND_."},
    {0,0,0}
  },
  .parse = &cmd_exec_parse,
  .call  = &cmd_exec_call,
  .free  = &cmd_exec_free
};

