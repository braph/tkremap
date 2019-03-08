#include "aliases.h"

alias_t **aliases;
size_t  aliases_size;

alias_t* alias_get(const char *name) {
  for (int i = aliases_size; i--;)
    if (streq(name, aliases[i]->name))
      return aliases[i];

  return NULL;
}

#define ALIAS_MAX_ARGS 256

char** alias_resolve(const char *name, int *argc_out, char *args[]) {
  alias_t *alias = alias_get(name);
  if (! alias)
    return NULL;

  int ai = 0;
  char *alias_args[ALIAS_MAX_ARGS];

  for (char **arg = alias->definition; *arg; ++arg)
    alias_args[ai++] = strdup(*arg);

  for (int i = 0; i < *argc_out; ++i)
    alias_args[ai++] = strdup(args[i]);

  alias_args[ai] = NULL;

  *argc_out = ai;

  char **cpy = malloc(*argc_out * sizeof(char*));
  memcpy(cpy, alias_args, *argc_out * sizeof(char*));
  return cpy;
}

void alias_register(const char *name, const char *desc, int argc, const char **definition) {
  alias_t *alias    = malloc(sizeof(*alias));
  alias->name       = strdup(name);
  alias->desc       = strdup(desc);
  alias->definition = immutable_array(argc, definition);

  aliases_size++;
  aliases = realloc(aliases, aliases_size * sizeof(alias_t));
  aliases[aliases_size - 1] = alias;
}

void alias_init() {
  alias_register("shell", "shell", 6, (const char*[]) { "readline", "-!", "-P", "exec -s '", "-A", "'" });
}

