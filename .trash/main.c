  printf("context %d\n", sizeof(context));
  printf("termios %d\n", sizeof(struct termios));
  printf("keymode %d\n", sizeof(keymode_t));
  printf("binding %d\n", sizeof(binding_t));
  printf("commands %d\n", sizeof(commands_t));
  return 0;

char* alias(const char *template, ...) {
  static char* buf;
  free(buf);
  buf = 0;

  if (template) {
    va_list ap;
    va_start(ap, template);
    vasprintf(&buf, template, ap);
    va_end(ap);
  }

  return buf;
}

void _sighandler(int sig) {
  //printf("RECEIVED %d, %s\n", sig, strsignal(sig));
  signal(sig, SIG_DFL);
  raise(sig);
}

signal(SIGTSTP, _sighandler);
signal(SIGSTOP, _sighandler);
signal(SIGCONT, _sighandler);
signal(SIGTTIN, _sighandler);
signal(SIGTTOU, _sighandler);

#define WITH_ARG2(...) \
  if (! (ok = (uintptr_t) (arg2 = argv[optind++]))) { \
    error_set_errno(E_MISSING_ARG); \
  } else { ok = __VA_ARGS__ }

