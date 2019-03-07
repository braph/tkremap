#ifndef _LEXER_H
#define _LEXER_H

#include <stdio.h>

#define LEX_ERROR                        -2

#define LEX_TOKEN_DOUBLE_QUOTE           10
#define LEX_ERROR_MISSING_DOUBLE_QUOTE  -10

#define LEX_TOKEN_SINGLE_QUOTE           11
#define LEX_ERROR_MISSING_SINGLE_QUOTE  -11

#define LEX_TOKEN_WORD                   12
#define LEX_TOKEN_END                    13

typedef struct lexer_t {
  FILE      *in;
  int        is_eof;
  int        line;
  int        line_pos;

  char      *token_buf;
  int        token_bufsz;
  int        token_pos;

  #define    LEX_BUF_SZ 8
  char       buf[LEX_BUF_SZ];
  int        buf_pos;

  int        error_num;
  char      *error_buf;
} lexer_t;

extern  lexer_t       *_current_lex;
#define lex_line      (_current_lex->line)
#define lex_line_pos  (_current_lex->line_pos)

int   lex_init(FILE *);
void  lex_destroy();
int   lex_lex();
int   lex_eof();
char* lex_token();
char* lex_error();

#endif
