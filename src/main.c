#include <stdio.h>

#include "arg_parse.h"
#include "parse_stream.h"

#include "imc/lexer.h"

struct state
{
    cstr filename;
};

static const char *DATA =
"   // this is a test\n"
"// another test!!\n"
"      //// and again!\n"
"++++ && +-/*()===\n";

static bool parse_args(struct state *state, int argc, char **argv)
{
    struct arg_conf args_arr[] =
    {
        {
            .string_val = &state->filename,
            .short_opt = 'i',
            .long_opt = "input",
            .description = "Input file to process.",
            .type = ARG_TYPE_ARG_REQUIRED,
        },
        {},
    };

    struct arg_parse parser =
    {
        .name = argv[0],
        .args = args_arr,
        .help = true,
    };

    if (!ARG_parse(parser, argc, argv))
    {
        return false;
    }

    if (cstr_is_empty(&state->filename))
    {
        printf("error: input file required (-i,--input)!!\n");
        return false;
    }

    return true;
}

void ps_test(struct state *state)
{
    struct parse_stream *ps = ps_new(cstr_str(&state->filename));

    if (!ps)
    {
        return;
    }

    while (!ps_is_eof(ps))
    {
        int pos = ps_position(ps);
        int lne = ps_line(ps);
        int col = ps_column(ps);
        char cur = ps_pop(ps);
        char next = ps_peek(ps);

        printf("%04d:%02d:%02d", pos, lne, col);

        switch (cur)
        {
            case '\0':
                printf("     ,");
                break;
            case ' ':
                printf(" ' ' ,");
                break;
            case '\t':
                printf("  \\t ,");
                break;
            case '\n':
                printf("  \\n ,");
                break;
            default:
                printf("   %c ,", cur);
        }

        switch (next)
        {
            case '\0':
                printf("\n");
                break;
            case ' ':
                printf(" ' '\n");
                break;
            case '\t':
                printf("  \\t\n");
                break;
            case '\n':
                printf("  \\n\n");
                break;
            default:
                printf("   %c\n", next);
        }
    }

    ps_free(ps);
}

void lex_test(struct state *)
{
    struct imc_lexer_itr *itr = IMC_LEX_begin_mem(DATA, strlen(DATA));

    int i = 0;

    while (!IMC_LEX_itr_is_eof(itr) && i < 10)
    {
        csview view = IMC_LEX_itr_get_sv_token(itr);
        // printf("%02lu: %s\n", IMC_LEX_itr_get_line(itr), IMC_LEX_itr_get_str_line(itr));
        printf("%02lu: " c_svfmt "\n", IMC_LEX_itr_get_line(itr), c_svarg(view));
        IMC_LEX_itr_next(itr);
        i++;
    }

    IMC_LEX_itr_free(itr);
}

int main(int argc, char **argv)
{
    struct state state =
    {
    };

    if (!parse_args(&state, argc, argv))
    {
        return EXIT_FAILURE;
    }

    // ps_test(&state);

    lex_test(&state);

    return 0;
}
