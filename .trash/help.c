static __attribute__((always_inline))
void pad_right(int n) {
  printf("%*s", n, "");
  //while (n--) putchar(' ');
}

static __attribute__((always_inline))
int has_more_lines(const char *s) {
  return !!strchr(s, '\n');
}

static char* indent(const char *str, int pad) {
  int   ind = -1;
  char *res = calloc(1, strlen(str) + 1024);

  for (int i = pad; i--; )
    res[++ind] =  ' ';

  do {
    res[++ind] = *str;
    if (*str == '\n')
      for (int i = pad; i--; )
        res[++ind] = ' ';
  } while (*++str);

  return res;
}

#define strndup(...) strndup_repl(__VA_ARGS__)
static char __inline *strndup_repl(const char *s, int size) {
  char *newstr = malloc(size + 1);
  snprintf(newstr, size, "%s", s);
  return newstr;
}

static char* firstline(const char *s) {
  return strndup(s, strcspn(s, "\n"));
}

static const char* __inline more_lines(const char *s) {
  return s += strcspn(s, "\n");
}
