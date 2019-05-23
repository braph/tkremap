// old version 1:
static
int load_conf_at(const char *dir, const char *f) {
  int  ret = 0;
  char oldcwd[8192];

  if (! getcwd(oldcwd, sizeof(oldcwd)) || chdir(dir))
    return error_set(errno, f), 0;

  if (! access(f, F_OK))
    ret = read_conf_file(f);
  else
    error_set(errno, f);

  if (chdir(oldcwd))
    fprintf(stderr, "chdir(%s): %s", oldcwd, strerror(errno));

  if (ret)
    debug("Loaded %s/%s", dir, f);
  return ret;
}

// old version 2: (with getcwd/chdir)
static int load_conf_at(const char *dir, const char *file) {
  int   ret = 0;
  char  olddir[PATH_MAX];
  FILE *fh;

  if (! getcwd(olddir, sizeof(olddir)) || chdir(dir))
    return error_set(errno, file), 0;

  if ((fh = fopen(file, "r"))) {
    ret = read_conf_stream(fh);
    fclose(fh);
  }
  else
    error_set(errno, file);

  if (chdir(olddir))
    fprintf(stderr, "%s: %s", olddir, strerror(errno));

  if (ret)
    debug("Loaded %s/%s", dir, file);
  return ret;
}
