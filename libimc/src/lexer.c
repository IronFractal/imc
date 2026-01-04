#define _GNU_SOURCE
#include "imc/lexer.h"

#include <stdio.h>

static bool utf8_iscased(uint32_t c) __attribute__((unused));
static bool utf8_isword(uint32_t c) __attribute__((unused));
static bool utf8_isblank(uint32_t c) __attribute__((unused));
static bool utf8_isspace(uint32_t c) __attribute__((unused));

#include <stc/csview.h>
#include <stc/utf8.h>
#include <stc/cstr.h>
#include <stc/priv/ucd_prv.c>

#define ARRLEN(ARR) (sizeof(ARR)/sizeof(ARR[0]))

enum imc_lex_state
{
    IMC_LS_GENERAL,
    IMC_LS_LIT_NUM,
    IMC_LS_LIT_STRING,
};

struct imc_lexer_itr
{
    bool is_eof;
    enum imc_token token_type;

    FILE *file;

    cstr cstr_line;

    size_t ind_line;
    size_t ind_column;

    csview sv_token;



    // cstr strtoken;
    // enum imc_token token;

    // size_t pos;

    // size_t token_begin;
};

struct imc_keywords
{
    const zsview string;
    enum imc_token token;
}
const KEYWORDS[] =
{
    {.string = c_zv("else"),  .token = IMC_TOKEN_ELSE},
    {.string = c_zv("end"),   .token = IMC_TOKEN_END},
    {.string = c_zv("false"), .token = IMC_TOKEN_LIT_BOOL},
    {.string = c_zv("for"),   .token = IMC_TOKEN_FOR},
    {.string = c_zv("if"),    .token = IMC_TOKEN_IF},
    {.string = c_zv("in"),    .token = IMC_TOKEN_IN},
    {.string = c_zv("true"),  .token = IMC_TOKEN_LIT_BOOL},
    {.string = c_zv("while"), .token = IMC_TOKEN_WHILE},
};

const zsview SYMBOLS_S[] =
{
    c_zv("("),
    c_zv(")"),
    c_zv("["),
    c_zv("]"),
    c_zv("+"),
    c_zv("-"),
    c_zv("="),
    c_zv("*"),
    c_zv("/"),
    c_zv("^"),
    c_zv("%"),
    c_zv("!"),
    c_zv("<"),
    c_zv(">"),
    c_zv(","),
};

const zsview SYMBOLS_D[] =
{
    c_zv("+="),
    c_zv("-="),
    c_zv("*="),
    c_zv("/="),
    c_zv("^="),
    c_zv("!="),
    c_zv("=="),
    c_zv("<="),
    c_zv(">="),
    c_zv("&&"),
    c_zv("||"),
};

static bool is_sym_s(csview sv)
{
    for (size_t i = 0; i < ARRLEN(SYMBOLS_S); i++)
    {
        if (csview_equals(sv, SYMBOLS_S[i].str))
        {
            return true;
        }
    }
    return false;
}

static bool is_sym_d(csview sv)
{
    for (size_t i = 0; i < ARRLEN(SYMBOLS_D); i++)
    {
        if (csview_equals(sv, SYMBOLS_D[i].str))
        {
            return true;
        }
    }
    return false;
}

struct imc_lexer_itr *IMC_LEX_begin(const char *filename)
{
    struct imc_lexer_itr *itr = calloc(1, sizeof(struct imc_lexer_itr));

    if (!itr)
    {
        goto itr_failure;
    }

    itr->file = fopen(filename, "r");

    if (!itr->file)
    {
        goto itr_failure;
    }

    if (!cstr_getline(&itr->cstr_line, itr->file))
    {
        goto itr_failure;
    }

    IMC_LEX_itr_next(itr);

    return itr;
itr_failure:
    IMC_LEX_itr_free(itr);
    return nullptr;
}

struct imc_lexer_itr *IMC_LEX_begin_mem(const char *str, size_t size)
{
    struct imc_lexer_itr *itr = calloc(1, sizeof(struct imc_lexer_itr));

    if (!itr)
    {
        goto itr_failure;
    }

    itr->file = fmemopen((void *)str, size, "r");

    if (!itr->file)
    {
        goto itr_failure;
    }

    if (!cstr_getline(&itr->cstr_line, itr->file))
    {
        goto itr_failure;
    }

    IMC_LEX_itr_next(itr);

    return itr;
itr_failure:
    IMC_LEX_itr_free(itr);
    return nullptr;
}

void IMC_LEX_itr_free(struct imc_lexer_itr *itr)
{
    if (itr)
    {
        fclose(itr->file);
        cstr_drop(&itr->cstr_line);

        itr->file = nullptr;
    }

    free(itr);
}

static bool next_token(csview *view, enum imc_token *token, size_t *ind)
{
    size_t before = csview_size(*view);
    csview_iter itr;
    uint32_t rune = 0;

    *token = IMC_TOKEN_UNKNOWN;

    *view = csview_trim_start(*view);

    *ind = before - csview_size(*view);

    if (csview_is_empty(*view))
    {
        return false;
    }

    if (csview_starts_with(*view, "//"))
    {
        *token = IMC_TOKEN_COMMENT;
        return true;
    }

    if (is_sym_d(csview_subview(*view, 0, 2)))
    {
        *view = csview_subview(*view, 0, 2);
        *token = IMC_TOKEN_SYMBOL;
        return true;
    }

    if (is_sym_s(csview_subview(*view, 0, 1)))
    {
        *view = csview_subview(*view, 0, 1);
        *token = IMC_TOKEN_SYMBOL;
        return true;
    }





    itr = csview_begin(view);

    for (size_t i = 0; i < ARRLEN(KEYWORDS); i++)
    {
        if (csview_starts_with(*view, KEYWORDS[i].string.str))
        {
            csview_iter end = csview_advance(itr, KEYWORDS[i].string.size);

            rune = utf8_peek(end.ref);

            if (!utf8_isalnum(rune))
            {
                *view = csview_subview(*view, 0, KEYWORDS[i].string.size);
                *token = KEYWORDS[i].token;
                return true;
            }
        }
    }

    rune = utf8_peek(itr.ref);

    while (!utf8_isspace(rune))
    {
        csview_next(&itr);
        rune = utf8_peek(itr.ref);
    }



    return true;
}

void IMC_LEX_itr_next(struct imc_lexer_itr *itr)
{
    if (IMC_LEX_itr_is_eof(itr))
    {
        return;
    }

    if (feof(itr->file))
    {
        itr->is_eof = true;
        cstr_clear(&itr->cstr_line);
        return;
    }

    while (!itr->is_eof)
    {
        itr->ind_column += csview_size(itr->sv_token);
        itr->sv_token = cstr_subview(&itr->cstr_line, itr->ind_column,
        cstr_size(&itr->cstr_line) - itr->ind_column);

        if (next_token(&itr->sv_token, &itr->token_type, &itr->ind_column))
        {
            return;
        }

        if (!cstr_getline(&itr->cstr_line, itr->file))
        {
            itr->is_eof = true;
            cstr_clear(&itr->cstr_line);
            return;
        }

        itr->ind_line++;
        itr->ind_column = 0;
    }
}

bool IMC_LEX_itr_is_eof(struct imc_lexer_itr *itr)
{
    if (!itr || !itr->file)
    {
        return true;
    }

    if (itr->is_eof)
    {
        return true;
    }

    return false;
}

size_t IMC_LEX_itr_get_line(struct imc_lexer_itr *itr)
{
    if (!itr)
    {
        return 0;
    }

    return itr->ind_line;
}

// size_t IMC_LEX_itr_get_column(struct imc_lexer_itr *itr)
// {
//     if (!itr)
//     {
//         return 0;
//     }

//     return itr->column;
// }

// enum imc_token IMC_LEX_itr_get_token(struct imc_lexer_itr *itr)
// {
//     if (!itr)
//     {
//         return IMC_TOKEN_UNKNOWN;
//     }

//     return itr->token;
// }

const char *IMC_LEX_itr_get_str_line(struct imc_lexer_itr *itr)
{
    if (!itr)
    {
        return "";
    }

    return cstr_str(&itr->cstr_line);
}

csview IMC_LEX_itr_get_sv_token(struct imc_lexer_itr *itr)
{
    if (!itr)
    {
        return c_sv("");
    }

    return itr->sv_token;
}
