#include "aliases.h"

//alias shell 'readline -P exec -p $1 $2 $@
char* alias_resolve(const char *definition, char *args[]) {
  char result[8192];
  char *r = result;
  char *c = definition;
  int n, from, to, count, nread;

  while (*c) {
    if (*c == '$') {
      ++c;
      nread = 0;

      if (*c == '$') {
        *r++ = *c++;
      }
      else if (*c == '@') {
        for (int i = 0; i < argc; ++i)
          r += sprintf(r, "%s", args[i]);
      }
      else if (1 == sscanf("$%d%n", &n, &nread)) {
        r += sprintf(r, "%s", args[n]);
      }
      else if (1 == sscanf("${@:%d}%n", &from, &nread)) {
        for (int i = from; i < argc; ++i)
          r += sprintf(r, "%s", args[n]);
      }
      else if (2 == sscanf("${@:%d:%d}%n"), &from, &count, &nread) {
        for (int i = from, to = from + count; i < to; ++i)
          r += sprintf(r, "%s", args[i]);
      }
      else
        return error_write(), NULL;

      c += nread;
    }
    else
      *r++ = *c++;
  }

  return strdup(result);
}

COMMAND_CALL_FUNC(cmd_alias_call) {
  char **args;
  unpackargs(NULL, &args, NULL, (command_args_t*) cmd->arg);

  aliases_t alias = {
    .name       = strdup(args[0]),
    .definition = strdup(args[1]),
    .desc       = strdup(args[2])
  };

  aliases = realloc(aliases, ++aliases_size * sizeof(void*));
  aliases[aliases_size - 1] = MEMDUP(&alias);
}

const command_t command_alias = {
  .name  = "alias",
  .desc  = "Create a command alias",
  .args  = ARGUMENTS("NAME", "COMMAND", "DESCRIPTION"),
  .parse = &copyargs,
  .call  = &cmd_alias_call,
  .free  = &deleteargs
};

  /* TODO: enable aliases in commands.c:
  // Resolve alias first
  char **aliased_argv = alias_resolve(name, &argc, args);
  if (aliased_argv) {
    command_call_t* ret = command_parse(argc, aliased_argv, store);
    error_add(name); // we want to know the aliased named
    freeArray(aliased_argv, argc);
    return ret;
  }
  */

