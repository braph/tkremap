#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>

lexer_t *_current_lex;
#define lex (*_current_lex)

int lex_init(FILE *in) {
  _current_lex = calloc(1, sizeof(lexer_t));
  if (! _current_lex)
    return 0;

  lex.in       = in;
  lex.line     = 1;
  lex.line_pos = 1;
  lex.buf_pos  = -1;

  return 1;
}

void lex_destroy() {
  free(lex.token_buf);
  free(lex.error_buf);
  free(_current_lex);
  _current_lex = NULL;
}

/*
 * We're handling the line continuation character (\) here,
 * so we don't have to care about it in lex()
 */
static int feed_buf() {
  int c1 = fgetc(lex.in);

  if (c1 == EOF || c1 == 0)
    return EOF;

  if (c1 == '\\') {
    int c2 = fgetc(lex.in);

    if (c2 == '\n') {
      lex.line++;
      lex.line_pos = 1;
      return feed_buf();
    }
    else if (c2 == EOF || c2 == 0) {
      lex.buf[++lex.buf_pos] = c1;
    }
    else {
      lex.buf[++lex.buf_pos] = c2;
      lex.buf[++lex.buf_pos] = c1;
    }
  }
  else {
    lex.buf[++lex.buf_pos] = c1;
  }

  return 1;
}

static int lex_peekc() {
  if (lex.buf_pos == -1) {
    if (feed_buf() == EOF)
      return EOF;
  }

  return lex.buf[lex.buf_pos];
}

static int lex_getc() {
  if (lex.buf_pos == -1) {
    if (feed_buf() == EOF)
      return EOF;
  }

  if (lex.buf[lex.buf_pos] == '\n')
    lex.line++, lex.line_pos = 1;
  else
    lex.line_pos++;

  return lex.buf[lex.buf_pos--];
}

static void lex_ungetc(int c) {
  lex.line_pos--;
  lex.buf[++lex.buf_pos] = c;
}

static void token_clear() {
  lex.token_pos = 0;
}

#define LEX_TOKEN_BUF_INC 1024
static void token_append(int c) {
  if (lex.token_pos == lex.token_bufsz) {
    lex.token_bufsz += LEX_TOKEN_BUF_INC;
    lex.token_buf = realloc(lex.token_buf, lex.token_bufsz);
  }

  lex.token_buf[lex.token_pos++] = c;
}

static void token_finalize() {
  token_append(0);
}

char* lex_token() {
  return lex.token_buf;
}

static unsigned int read_hex() {
  int  c;
  unsigned int val = 0;

  while (isxdigit((c = lex_getc())))
    val = val * 16 + 
      (c <= '9' ? c - '0'      :
      (c <= 'F' ? c - 'A' + 10 :
       c - 'a' + 10));
  lex_ungetc(c);
  return val;
}

static unsigned int read_oct() {
  int  c;
  unsigned int val = 0;

  while ((c = lex_getc()) >= '0' && c < '8')
    val = val * 8 + c - '0';
  lex_ungetc(c);
  return val;
}

static unsigned int read_dec() {
  int c;
  unsigned val = 0;

  while (isdigit((c = lex_getc())))
    val = val * 10 + c - '0';
  lex_ungetc(c);
  return val;
}

static int read_double_quote() {
  int  c;

  token_clear();
  while ((c = lex_getc()) != EOF) {
    if (c == '"')
      return token_finalize(), LEX_TOKEN_DOUBLE_QUOTE;
    else if (c == '\\') {
      switch ((c = lex_getc())) {
        case 'a':   token_append('\a'); break;
        case 'b':   token_append('\b'); break;
        case 't':   token_append('\t'); break;
        case 'n':   token_append('\n'); break;
        case 'v':   token_append('\v'); break;
        case 'f':   token_append('\f'); break;
        case 'r':   token_append('\r'); break;
        case 'e':   token_append(033);  break;
        case 'x':   token_append(read_hex()); break;
        case '0':   token_append(read_oct()); break;
        case '1': case '2': case '3':
        case '4': case '5': case '6':
        case '7': case '8': case '9':
                    token_append(read_dec()); break;
        default:    token_append(c);
      }
    }
    else
      token_append(c);
  }

  lex.error_num = LEX_ERROR_MISSING_DOUBLE_QUOTE;
  return LEX_ERROR;
}

static int read_single_quote() {
  int c;

  token_clear();
  while ((c = lex_getc()) != EOF) {
    if (c == '\'')
      return token_finalize(), LEX_TOKEN_SINGLE_QUOTE;
    else if (c == '\\' && lex_peekc() == '\'')
      token_append(lex_getc());
    else
      token_append(c);
  }

  lex.error_num = LEX_ERROR_MISSING_SINGLE_QUOTE;
  return LEX_ERROR;
}

static void consume_comment() {
  int c;

  while ((c = lex_getc()) != EOF)
    if (c == '\n')
      break;
}

static int read_word() {
  int c;

  token_clear();
  while ((c = lex_getc()) != EOF) {
    if (c == '\\' && lex_peekc() != EOF) {
      token_append(c);
      token_append(lex_getc());
    }
    else if (c == '\'' || c == '"' || c == ';' || isspace(c)) {
      lex_ungetc(c);
      break;
    }
    else {
      token_append(c);
    }
  }
  token_finalize();

  return LEX_TOKEN_WORD;
}

int lex_lex() {
  int c = lex_getc();

  switch (c) {
    case '"':   return read_double_quote();
    case '\'':  return read_single_quote();
    case ';':
    case '\n':  return LEX_TOKEN_END;
    case '#':   consume_comment(); /* fall through */
    case ' ':
    case '\t':  return lex_lex();
    case EOF:   lex.is_eof = 1;
                return EOF;
    default:    lex_ungetc(c);
                return read_word();
  }
}

int lex_eof() {
  return lex.is_eof;
}

#define LEX_ERROR_BUF_SZ 1024
char *lex_error() {
  if (! lex.error_buf)
    lex.error_buf = malloc(LEX_ERROR_BUF_SZ);

  sprintf(lex.error_buf, "%d:%d: ", lex.line, lex.line_pos);

  switch (lex.error_num) {
    case 0:
      return strcat(lex.error_buf, strerror(0));
    case LEX_ERROR_MISSING_SINGLE_QUOTE:
      return strcat(lex.error_buf, "unterminated single quote");
    case LEX_ERROR_MISSING_DOUBLE_QUOTE:
      return strcat(lex.error_buf, "unterminated double quote");
    default:
      return strcat(lex.error_buf, "error in lexer");
  }
}
