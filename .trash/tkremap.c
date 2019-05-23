#define OPTION_FLAG(COMMAND_OPT) \
  COMMAND_OPT.opt

#define OPTION_META(COMMAND_OPT) \
  COMMAND_OPT.meta

#define OPTION_DESC(COMMAND_OPT) \
  COMMAND_OPT.desc


// !---------------------------------------------------------------------------
void keymode_init(keymode_t *km, const char *name) {
  km->name = strdup(name);
  km->root = calloc(1, sizeof(binding_t));
}

keymode_t* add_keymode(const char *name) {
  keymode_t *km = calloc(1, sizeof(*km));
  keymode_init(km, name);

  context.n_keymodes++;
  context.keymodes = realloc(context.keymodes, context.n_keymodes * sizeof(km));
  context.keymodes[context.n_keymodes - 1] = km;
  return km;
}
// !---------------------------------------------------------------------------

void* copyargs(int argc, char *args[], option *options) {
  command_args_t *cmdargs = malloc(sizeof(command_args_t));

  int size = argc + 1;
  for (int i = 0; i < argc; ++i)
    size += strlen(args[i]);

  char **args2 = malloc((argc + 1) * sizeof(char*));
  args2[argc] = NULL;

  char *strings = calloc(1, size);
  for (int i = 0; i < argc; ++i) {
    strcpy(strings, args[i]);
    args2[i] = strings;
    strings += strlen(args[i]) + 1;
  }

  cmdargs->argc = argc;
  cmdargs->args = args2;
  return cmdargs;
}

// old way

  static
void *redirect_to_stdout(void *_fd)
{
  #define  REDIRECT_BUFSZ 4096
  int      fd = *((int*)_fd);
  char     buffer[REDIRECT_BUFSZ];
  char     *b;
  ssize_t  bytes_read;
  ssize_t  bytes_written;
  struct   pollfd fds = { .fd = fd, .events = POLLIN };

  for (;;) {
    if (poll(&fds, 1, 100) > 0 && fds.revents & POLLIN ) {
      if (context.stop_output)
        return NULL;
    }
    else {
      if (context.stop_output)
        return NULL;
      continue;
    }

    if ((bytes_read = read(fd, &buffer, REDIRECT_BUFSZ)) == -1) {
      if (errno == EAGAIN) {
        usleep(100);
        continue;
      }
      else
        return NULL;
    }

    b = buffer;
    for (int i = 5; i--; ) {
      bytes_written = write(STDOUT_FILENO, b, bytes_read);

      if (bytes_written >= 0) {
        b += bytes_written;
        bytes_read -= bytes_written;
        if (bytes_read == 0)
          break;
      }
      else if (bytes_written == -1 && errno != EAGAIN)
        break;

      usleep(100);
    }
  }
}

int start_program_output() {
  context.stop_output = 0;
  /*
  if ((errno = pthread_create(&context.redir_thread,
          NULL, redirect_to_stdout, (void*)&context.program_fd)))
    return 0;
  */
  return 1;
}

void stop_program_output() {
  context.stop_output = 1;
  //pthread_join(context.redir_thread, NULL);
}

