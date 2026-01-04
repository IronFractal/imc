#include <stdio.h>

#include "langvm.h"
#include "arg_parse.h"

struct state
{
    cstr filename;
};

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

int main(int argc, char **argv)
{
    struct imc_lang_vm *vm;
    struct state state =
    {
    };

    if (!parse_args(&state, argc, argv))
    {
        return EXIT_FAILURE;
    }

    vm = IMC_VM_new();

    IMC_VM_run_src_file(vm, cstr_str(&state.filename));

    IMC_VM_write_png(vm, "test.png");

    IMC_VM_free(vm);
    return 0;
}
