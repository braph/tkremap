#include "conf.h"
#include "tkremap.h"

#include <err.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define CFG_DIR_NAME "tkremap"

static
int load_conf_at(const char *dir, const char *f) {
   int  ret = 0;
   char oldcwd[4096];

   if (! getcwd(oldcwd, sizeof(oldcwd)) || chdir(dir))
      return 0;

   if (! access(f, F_OK))
      ret = read_conf_file(f);
   else
      error_write("%s: %s", strerror(ENOENT), f);

   if (chdir(oldcwd))
      warn("chdir(%s)", oldcwd);

   return ret;
}

int load_conf(const char *f) {
   char dir[4096];
   const char *xdg_home, *home;

   if (strchr(f, '/'))
      return read_conf_file(f);

   if (read_conf_file(f))
      return 1;

   if ((xdg_home = getenv("XDG_CONFIG_HOME"))) {
      sprintf(dir, "%s/%s", xdg_home, CFG_DIR_NAME);
      if (load_conf_at(dir, f))
         return 1;
   }

   if ((home = getenv("HOME"))) {
      sprintf(dir, "%s/.config/%s", home, CFG_DIR_NAME);
      if (load_conf_at(dir, f))
         return 1;

      sprintf(dir, "%s/.%s", home, CFG_DIR_NAME);
      if (load_conf_at(dir, f))
         return 1;
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
   .name  = "load",
   .desc  = "Load configuration file\n"
            "If file is a sole filename it will be\n"
            "searched in the following places:\n"
            " - $PWD\n"
            " - $XDG\\_CONFIG\\_HOME/" CFG_DIR_NAME "\n"
            " - $HOME/.config/" CFG_DIR_NAME "\n"
            " - $HOME/." CFG_DIR_NAME,
   .args  = (const char*[]) { "FILE", 0 },
   .call  = &call,
   .parse = &parse,
   .free  = &free
};
