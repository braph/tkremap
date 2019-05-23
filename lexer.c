#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

lexer_t      *_current_lex;
#define lex (*_current_lex)

static int feed_buf();

int lex_init(FILE *in) {
  _current_lex = calloc(1, sizeof(lexer_t));
  if (! _current_lex)
    return 0;

  lex.in = in;
  feed_buf();
  return 1;
}

void lex_destroy() {
  free(lex.buf);
  free(lex.token_buf);
  free(_current_lex);
  _current_lex = NULL;
}

/*
 * Read whole configuration line.
 */
static int feed_buf() {
  int n = getline(&lex.buf, &lex.bufsz, lex.in);
  if (n < 1)
    return (lex.error_num = EOF);

  lex.line_no++;
  for (;;) {
    if (lex.buf[--n] != '\n' || !n) // no newline || stripped size == 0
      break;

    if (lex.buf[n - 1] == '\\') {
      int read_another_line = 1;
      for (int i = n - 1; i-- ;) {
        if (lex.buf[i] == '\\')
          read_another_line = !read_another_line;
        else
          break;
      }

      if (read_another_line) {
        lex.buf[n] = '\0';   // kill newline
        lex.buf[--n] = '\0'; // kill backslash

        lex.line_no++;
        char   *next_line = NULL;
        size_t  next_line_bufsz = 0;
        ssize_t next_line_sz;
        if ((next_line_sz = getline(&next_line, &next_line_bufsz, lex.in)) < 1) {
          free(next_line);
          break;
        }
        else {
          lex.buf = realloc(lex.buf, n + next_line_sz + 1);
          strcat(lex.buf, next_line);
          free(next_line);
          n += next_line_sz;
          lex.bufsz += next_line_sz;
        }
      }
    }
  }

  lex.c = lex.buf;
  return *lex.c;
}

/* Returns current character or EOF */
static int lex_peekc() {
  return (*lex.c ? *lex.c : feed_buf());
}

/* Returns current character or EOF, moves to next character */
static int lex_getc() {
  int c = (*lex.c ? *lex.c : feed_buf());
  lex.c++;
  return c;
}

#define lex_unget() \
  (--lex.c)

#define issyntax(C) \
  (!!strchr("'\";{}#|&", C))

#define token_clear() \
  (lex.token_pos = 0)

#define token_finalize() \
  (token_append(0))

#define LEX_TOKEN_BUF_INC 256
static void token_append(int c) {
  if (lex.token_pos == lex.token_bufsz) {
    lex.token_bufsz += LEX_TOKEN_BUF_INC;
    lex.token_buf = realloc(lex.token_buf, lex.token_bufsz);
  }

  lex.token_buf[lex.token_pos++] = c;
}

static int read_double_quote() {
  int c;
  unsigned val = 0;
  token_clear();

  for (;;) {
    c = lex_getc();
HAVE_CHAR:

    if (c == '\\') {
      switch ((c = lex_getc())) {
        case '1': case '2': case '3':
        case '4': case '5': case '6':
        case '7': case '8': case '9':
          do {
            val = val * 10 + c - '0';
          } while (isdigit(c = lex_getc()));
          token_append(val);
          goto HAVE_CHAR;
        case 'x':
          while (isxdigit((c = lex_getc())))
            val = val * 16 + 
              (c <= '9' ? c - '0'      :
              (c <= 'F' ? c - 'A' + 10 :
               c - 'a' + 10));
          token_append(val);
          goto HAVE_CHAR;
        case '0':
          while ((c = lex_getc()) >= '0' && c < '8')
            val = val * 8 + c - '0';
          token_append(val);
          goto HAVE_CHAR;
        #define case break;case
        case 'a': c = '\a';
        case 'b': c = '\b';
        case 't': c = '\t';
        case 'n': c = '\n';
        case 'v': c = '\v';
        case 'f': c = '\f';
        case 'r': c = '\r';
        case 'e': c = '\033';
        #undef case
      }
    }
    else if (c == '"') {
      token_finalize();
      return LEX_TOKEN_DOUBLE_QUOTE;
    }

    if (c == EOF)
      return (lex.error_num = LEX_ERROR_MISSING_DOUBLE_QUOTE);

    token_append(c);
  }
}

static int read_single_quote() {
  int c;

  token_clear();
  for (;;) {
    switch (c = lex_getc()) {
      case '\'':
        token_finalize();
        return LEX_TOKEN_SINGLE_QUOTE;
      case '\\':
        c = lex_getc();
    }

    if (c == EOF)
      return (lex.error_num = LEX_ERROR_MISSING_SINGLE_QUOTE);
    token_append(c);
  }
}

static int read_word() {
  int c;

  token_clear();
  while ((c = lex_getc()) != EOF) {
    if (c == '\\' && lex_peekc() != EOF) {
      token_append(c);
      token_append(lex_getc());
    }
    else if (isspace(c) || issyntax(c)) {
      lex_unget();
      break;
    }
    else {
      token_append(c);
    }
  }
  token_finalize();

  return LEX_TOKEN_WORD;
}

#define SINGLE_TOK(C) do { \
  token_clear(); \
  token_append(C); \
  token_finalize(); \
} while(0)

int lex_lex() {
  int c;

  if (lex.unlexed) {
    lex.unlexed = 0;
    return lex.token_type;
  }
  else if (lex_eof())
    return (lex.token_type = EOF);

  while (1) {
    switch (c = lex_getc()) {
      case '{':  
        SINGLE_TOK('{');
        return (lex.token_type = LEX_TOKEN_BLOCK_BEG);

      case '}':  
        SINGLE_TOK('}');
        return (lex.token_type = LEX_TOKEN_BLOCK_END);

      case '&': 
        if (lex_getc() == '&')
          return (lex.token_type = LEX_TOKEN_AND);
        else
          return LEX_ERROR_UNEXPECTED_SYMBOL;

      case '|': 
        if (lex_getc() == '|')
          return (lex.token_type = LEX_TOKEN_OR);
        else
          return LEX_ERROR_UNEXPECTED_SYMBOL;

      case '"':   return (lex.token_type = read_double_quote());
      case '\'':  return (lex.token_type = read_single_quote());
      case ';':   return (lex.token_type = LEX_TOKEN_SEMICOLON);
      case '\n':  return (lex.token_type = LEX_TOKEN_NEW_LINE);
      case '#':   while ((c = lex_getc()) != '\n' && c != EOF); break; /* fall through */
      case ' ':
      case '\t':  break;
      case EOF:   return (lex.token_type = lex.error_num = EOF);
      default:    lex_unget();
                  return (lex.token_type = read_word());
    }
  }
}

int   lex_char_pos()  { return (lex.c - lex.buf); }
int   lex_line_no()   { return lex.line_no;       }
char* lex_line()      { return lex.buf;           }
char* lex_token()     { return lex.token_buf;     }
void  lex_unlex()     { lex.unlexed = 1;          }
int   lex_eof()       { return lex.error_num;     }

int lex_errorno() {
  return (lex.error_num != EOF ? lex.error_num : 0);
}

char *lex_error() {
  switch (lex.error_num) {
    case LEX_ERROR_MISSING_SINGLE_QUOTE: return "unterminated single quote";
    case LEX_ERROR_MISSING_DOUBLE_QUOTE: return "unterminated double quote";
    case LEX_ERROR_UNEXPECTED_SYMBOL:    return "unexpected symbol";
    case EOF:
    case 0:  return strerror(0);
    default: return NULL;
  }
}
