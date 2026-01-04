#ifndef IMC_LEXER_H
#include <stdio.h>

#include <stc/csview.h>

enum imc_token
{
    IMC_TOKEN_UNKNOWN,
    IMC_TOKEN_COMMENT,
    IMC_TOKEN_LIT_NUM,
    IMC_TOKEN_LIT_BOOL,
    IMC_TOKEN_LIT_STRING,
    IMC_TOKEN_IDEN,
    IMC_TOKEN_SYMBOL,
    IMC_TOKEN_IF,
    IMC_TOKEN_ELSE,
    IMC_TOKEN_WHILE,
    IMC_TOKEN_FOR,
    IMC_TOKEN_IN,
    IMC_TOKEN_END,
    IMC_TOKEN_EOF,
};

struct imc_lexer_itr;

struct imc_lexer_itr *IMC_LEX_begin(const char *filename);

struct imc_lexer_itr *IMC_LEX_begin_mem(const char *str, size_t size);

void IMC_LEX_itr_free(struct imc_lexer_itr *itr);

void IMC_LEX_itr_next(struct imc_lexer_itr *itr);

bool IMC_LEX_itr_is_eof(struct imc_lexer_itr *itr);

size_t IMC_LEX_itr_get_line(struct imc_lexer_itr *itr);

// size_t IMC_LEX_itr_get_column(struct imc_lexer_itr *itr);

// enum imc_token IMC_LEX_itr_get_token(struct imc_lexer_itr *itr);

const char *IMC_LEX_itr_get_str_line(struct imc_lexer_itr *itr);

csview IMC_LEX_itr_get_sv_token(struct imc_lexer_itr *itr);

#endif
