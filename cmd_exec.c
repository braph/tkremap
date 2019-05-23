#include "tkremap.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#ifndef _PATH_DEVNULL
#define _PATH_DEVNULL "/dev/null"
#endif

#define STREQ_EXCLAMATION(BUF) (BUF[0] == '!' && BUF[1] == '\0')
#define STREQ_MINUS(BUF)       (BUF[0] == '-' && BUF[1] == '\0')
#define MODE               (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH)

#define CMD_EXEC_REDIRECT_NULL  ((char*) 1)
#define CMD_EXEC_REDIRECT_STDIN ((char*) 2)

#define FREE_FILE(FILE) do { \
  if (FILE > CMD_EXEC_REDIRECT_STDIN) \
    free(FILE); \
  } while(0)

#define PARSE_FILE(FILE) \
  (STREQ_MINUS(FILE)       ? CMD_EXEC_REDIRECT_STDIN : \
  (STREQ_EXCLAMATION(FILE) ? CMD_EXEC_REDIRECT_NULL  : \
   strdup(FILE)))

typedef struct cmd_exec_args {
  uint8_t  use_shell         : 1;
  uint8_t  background        : 1;
  uint8_t  ignore_exitstatus : 1;
  char    *in;
  char    *err;
  char    *out;
  char  **args;
} cmd_exec_args;

static COMMAND_CALL_FUNC(cmd_exec_call) {
  cmd_exec_args *cmd_args = (cmd_exec_args*) cmd->arg;
  int pid;

  if ((pid = fork()) == 0) {
    int outfd = -1;
    int errfd = -1;
    int infd  = -1;

    if (cmd_args->out == CMD_EXEC_REDIRECT_STDIN)
      outfd = context.program_fd;
    else if (cmd_args->out == CMD_EXEC_REDIRECT_NULL)
      outfd = open(_PATH_DEVNULL, O_WRONLY, 0);
    else if (cmd_args->out)
      outfd = open(cmd_args->out, O_WRONLY|O_CREAT, MODE);

    if (cmd_args->err == CMD_EXEC_REDIRECT_STDIN)
      errfd = context.program_fd;
    else if (cmd_args->err == CMD_EXEC_REDIRECT_NULL)
      errfd = open(_PATH_DEVNULL, O_WRONLY, 0);
    else if (cmd_args->err)
      errfd = open(cmd_args->err, O_WRONLY|O_CREAT, MODE);

    if (cmd_args->in) {
      infd = open(cmd_args->in, O_RDONLY);
    }

    if (outfd >= 0) {
      close(STDOUT_FILENO);
      dup2(outfd, STDOUT_FILENO);
    }

    if (errfd >= 0) {
      close(STDERR_FILENO);
      dup2(errfd, STDERR_FILENO);
    }

    if (infd >= 0)
      dup2(infd, STDIN_FILENO);
    else if (cmd_args->background)
      close(STDIN_FILENO);

    if (cmd_args->use_shell)
      return execlp("/bin/sh", "sh", "-c", cmd_args->args[0], NULL), 0;
    else
      return execvp(cmd_args->args[0], cmd_args->args), 0;
  }
  else if (pid > 0) {
    if (! cmd_args->background) {
      int ret;
      waitpid(pid, &ret, 0);

      if (cmd_args->ignore_exitstatus)
        return 1;
      else
        return !WEXITSTATUS(ret);
    }

    return 1;
  }

  return 0;
}

static COMMAND_PARSE_FUNC(cmd_exec_parse) {
  cmd_exec_args *cmd_args = calloc(1, sizeof(*cmd_args));

  for (option *opt = options; opt->opt; ++opt) {
    #define case break; case
    switch (opt->opt) {
      case 'O': FREE_FILE(cmd_args->out);
                cmd_args->out = PARSE_FILE(opt->arg);
      case 'E': FREE_FILE(cmd_args->err);
                cmd_args->err = PARSE_FILE(opt->arg);
      case 'I': FREE_FILE(cmd_args->in);
                cmd_args->in  = PARSE_FILE(opt->arg);
      case 'b': cmd_args->background = 1;
      case 'x': cmd_args->ignore_exitstatus = 1;
      case 's':
        if (argc > 1)
          return error_set(E2BIG, "Only one argument allowed with -s"), NULL;
        cmd_args->use_shell = 1;
    }
    #undef case
  }

  cmd_args->args = immutable_array(argc, args);
  return cmd_args;
}

static void cmd_exec_free(void *_arg) {
  cmd_exec_args *cmd_args = (cmd_exec_args*)_arg;
  free(cmd_args->args);
  FREE_FILE(cmd_args->out);
  FREE_FILE(cmd_args->err);
  FREE_FILE(cmd_args->in);
  free(cmd_args);
}

// Output options: {STDOUT|STDERR}, FILE, TO PROGRAM STDIN, SUPPRESS

const command_t command_exec = {
  .name  = "exec"
    "\0Call external program\n"
    "Returns *TRUE* if command succeeded, *FALSE* on failure",
  .args  = "COMMAND\0*ARGS\0",
  .opts  = OPTIONS(
    OPTION('s', NO_ARG, "Pass _COMMAND_ to /bin/sh instead of invoking it using *exec(3)*"),
    OPTION('b', NO_ARG, "Run _COMMAND_ in background"),
    OPTION('O', "FILE", "Redirect STDOUT of _COMMAND_ to _FILE_.\n"
                        "Pass `*-*` for redirecting to program's *STDIN*\n"
                        "Pass `*!*` for discarding output"),
    OPTION('E', "FILE", "Redirect STDERR of _COMMAND_ to _FILE_.\n"
                        "Pass `*-*` for redirecting to program's *STDIN*\n"
                        "Pass `*!*` for discarding output"),
    OPTION('I', "FILE", "Use _FILE_ as STDIN for _COMMAND_."),
    OPTION('x', NO_ARG, "Ignore commands exit status, always return *TRUE*")
    //OPTION('Q', "Silence all"),
    //OPTION('q', "Silence stderr"),
  ),
  .parse = &cmd_exec_parse,
  .call  = &cmd_exec_call,
  .free  = &cmd_exec_free
};

