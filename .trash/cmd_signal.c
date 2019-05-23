#define STRCASEQ_SIG(S) \
  ((S[0]=='s'||S[0]=='S') && (S[1]=='i'||S[1]=='I') && (S[2]=='g'||S[2]=='G'))

static int name2signal(const char *name) {
  if (STRCASEQ_SIG(name))
    name += 3;

  for (int i = SIGNAL_SIZE; i--; )
    if (! strcasecmp(name, signals[i].name))
      return signals[i].number;

  return 0;
}
