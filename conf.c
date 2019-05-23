#include "conf.h"
#include "lexer.h"
#include "common.h"
#include "tkremap.h"
#include "commands.h"
#include "errormsg.h"
#include "termkeystuff.h"

#include <string.h>

int read_conf_stream(FILE *fh) {
  int            ret = 1;
  int            ttype;
  command_call_t call;
  keymode_t     *mode_restore = context.current_mode;

  lex_init(fh);

  while ((ttype = lex_lex()) != EOF) {
    // Skip newlines
    if (ttype == LEX_TOKEN_NEW_LINE)
      continue;

    // We're not on the EOF, put back the token for `command_parse`
    lex_unlex();

    // Parse command
    debug("read_conf_stream(): invoking command_parse() ...");
    if (! command_parse(NULL, &call)) {
      //error_addf("on line %d:%d", lex_line_no(), lex_char_pos()); TODO
      ret = 0;
      break;
    }
    debug("read_conf_stream(): parsed %s", call.command->name);

    // Call command
    ret = call.command->call(&call, NULL);
    command_call_free(&call);
    if (! ret) {
      //error_addf("on line %d:%d", lex_line_no(), lex_char_pos()); TODO
      break;
    }

    // Command must be terminated with ';', '\n' or EOF
    ttype = lex_lex();
    if (ttype != LEX_TOKEN_SEMICOLON && ttype != LEX_TOKEN_NEW_LINE && ttype != EOF) {
      debug("read_conf_stream(): type is %s", LEX_TOK2STR(ttype));
      error_set(E_SYNTAX, "Expected ';', '\\n' or EOF");
      ret = 0;
      break;
    }
  }

  lex_destroy();
  context.current_mode = mode_restore;
  return ret;
}

int read_conf_string(const char *str) {
  FILE *fh = fmemopen((void*) str, strlen(str), "r");

  if (! fh)
    return error_set_errno(errno), 0;

  int ret = read_conf_stream(fh);
  fclose(fh);
  return ret;
}

int read_conf_file(const char *file) {
  FILE *fh = fopen(file, "r");

  if (! fh)
    return error_set_errno(errno), 0;

  int ret = read_conf_stream(fh);
  fclose(fh);
  return ret;
}

