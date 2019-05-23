#include "tkremap.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define CFG_DIR_NAME  "tkremap"
#define CFG_EXTENSION "conf"

static int load_conf_at(const char *dir, const char *file) {
  int   ret = 0;
  char  filebuf[PATH_MAX * 2 + 2];
  FILE *fh;

  snprintf(filebuf, sizeof(filebuf), "%s/%s", dir, file);

  if ((fh = fopen(filebuf, "r"))) {
    ret = read_conf_stream(fh);
    fclose(fh);
  }
  else
    error_set(errno, filebuf);

  if (ret)
    debug("Loaded %s/%s", dir, filebuf);

  return ret;
}

int load_conf(const char *file) {
  char dirbuf[PATH_MAX];
  char filebuf[PATH_MAX];
  const char *home;

  // It's not a sole filename, do not try to load it from $HOME etc.
  if (strchr(file, '/'))
    return read_conf_file(file);

AGAIN:
  if (read_conf_file(file))
    return 1;

  if ((home = getenv("XDG_CONFIG_HOME"))) {
    snprintf(dirbuf, sizeof(dirbuf), "%s/%s", home, CFG_DIR_NAME);
    if (load_conf_at(dirbuf, file))
      return 1;
  }

  if ((home = getenv("HOME"))) {
    snprintf(dirbuf, sizeof(dirbuf), "%s/.config/%s", home, CFG_DIR_NAME);
    if (load_conf_at(dirbuf, file))
      return 1;

    snprintf(dirbuf, sizeof(dirbuf), "%s/.%s", home, CFG_DIR_NAME);
    if (load_conf_at(dirbuf, file))
      return 1;
  }

  // Try $file.CFG_EXTENSION
  if (! strsuffix(file,             2+("%s." CFG_EXTENSION))) {
    snprintf(filebuf, sizeof(filebuf), "%s." CFG_EXTENSION, file);
    file = filebuf;
    goto AGAIN;
  }

  return 0;
}

static COMMAND_CALL_FUNC(call) {
  return load_conf((char*) cmd->arg);
}

static COMMAND_PARSE_FUNC(parse) {
  return (void*) strdup(args[0]);
}

const command_t command_load = {
  .name  = "load"
    "\0Load configuration file\n\n"
    "If file is a sole filename it will be\n"
    "searched in the following directories:\n"
    " - *$PWD*\n"
    " - *$XDG\\_CONFIG\\_HOME*/" CFG_DIR_NAME "\n"
    " - *$HOME*/.config/"        CFG_DIR_NAME "\n"
    " - *$HOME*/."               CFG_DIR_NAME "\n"
    "The extension *." CFG_EXTENSION "* will be added if missing",
  .args  = "FILE\0",
  .call  = &call,
  .parse = &parse,
  .free  = &free
};
