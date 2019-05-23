#ifndef _LEXER_H
#define _LEXER_H

#include <stdio.h>

#define LEX_ERROR_MISSING_DOUBLE_QUOTE  -2
#define LEX_ERROR_MISSING_SINGLE_QUOTE  -3
#define LEX_ERROR_UNEXPECTED_SYMBOL     -4

#define LEX_TOKEN_DOUBLE_QUOTE           4 // 2
#define LEX_TOKEN_SINGLE_QUOTE           4 // 3
#define LEX_TOKEN_WORD                   4
#define LEX_TOKEN_SEMICOLON              5
#define LEX_TOKEN_AND                    6
#define LEX_TOKEN_OR                     7
#define LEX_TOKEN_BLOCK_BEG              8
#define LEX_TOKEN_BLOCK_END              9
#define LEX_TOKEN_NEW_LINE               10

#define LEX_TOK2STR(TOK) \
  ( TOK == EOF ? "EOF" : \
  ( TOK == LEX_TOKEN_WORD ? "LEX_TOKEN_WORD" : \
  ( TOK == LEX_TOKEN_SEMICOLON ? "LEX_TOKEN_SEMICOLON" : \
  ( TOK == LEX_TOKEN_AND ? "LEX_TOKEN_AND" : \
  ( TOK == LEX_TOKEN_OR ? "LEX_TOKEN_OR" : \
  ( TOK == LEX_TOKEN_BLOCK_BEG ? "LEX_TOKEN_BLOCK_BEG" : \
  ( TOK == LEX_TOKEN_BLOCK_END ? "LEX_TOKEN_BLOCK_END" : \
  ( TOK == LEX_TOKEN_NEW_LINE ? "LEX_TOKEN_NEW_LINE" : \
    "UNKNOWN" ))))))))

typedef struct lexer_t {
  FILE      *in;
  int        line_no;
  int        unlexed;

  char      *token_buf;
  int        token_bufsz;
  int        token_pos;
  int        token_type;

  char      *c;
  char      *buf;
  size_t     bufsz;

  int        error_num;
} lexer_t;

extern  lexer_t  *_current_lex;

int   lex_init(FILE *);
void  lex_destroy();
int   lex_lex();
void  lex_unlex();
int   lex_eof();
char* lex_line();
int   lex_line_no();
int   lex_char_pos();
int   lex_errorno();
char* lex_error();
char* lex_token();

#endif
