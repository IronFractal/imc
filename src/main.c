#include <stdio.h>

#include <stc/csview.h>

#include "langvm.h"
#include "arg_parse.h"

enum file_format
{
    FORMAT_UNKNOWN,
    FORMAT_PNG,
    FORMAT_JPG,
    FORMAT_BMP,
    FORMAT_TGA,
    FORMAT_XPM,
};

struct state
{
    cstr input_file;
    cstr output_file;
    enum file_format format;
};

struct lookup_entry
{
    const char *file_ext;
    enum file_format format;
} static const FORMAT_LOOKUP[] =
{
    {
        .file_ext = "png",
        .format = FORMAT_PNG,
    },
    {
        .file_ext = "jpg",
        .format = FORMAT_JPG,
    },
    {
        .file_ext = "jpeg",
        .format = FORMAT_JPG,
    },
    {
        .file_ext = "bmp",
        .format = FORMAT_BMP,
    },
    {
        .file_ext = "tga",
        .format = FORMAT_TGA,
    },
    {
        .file_ext = "xpm",
        .format = FORMAT_XPM,
    },
};

static bool parse_args(struct state *state, int argc, char **argv)
{
    struct arg_conf args_arr[] =
    {
        {
            .string_val = &state->input_file,
            .short_opt = 'i',
            .long_opt = "input",
            .description = "Input file to process.",
            .type = ARG_TYPE_ARG_REQUIRED,
        },
        {
            .string_val = &state->output_file,
            .short_opt = 'o',
            .long_opt = "output",
            .description = "Image file output.",
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

    cstr output_lower;

    if (!ARG_parse(parser, argc, argv))
    {
        return false;
    }

    if (cstr_is_empty(&state->input_file))
    {
        printf("error: input file required (-i,--input)!!\n");
        return false;
    }
    else if (cstr_is_empty(&state->output_file))
    {
        printf("error: output file required (-o,--output)!!\n");
        return false;
    }

    output_lower = cstr_tolower(cstr_str(&state->output_file));

    for (int i = 0; state->format == FORMAT_UNKNOWN && FORMAT_LOOKUP[i].file_ext; i++)
    {
        if (cstr_ends_with(&output_lower, FORMAT_LOOKUP[i].file_ext))
        {
            const isize no_ext_size = cstr_size(&output_lower) - strlen(FORMAT_LOOKUP[i].file_ext);
            csview no_ext = cstr_subview(&output_lower, 0, no_ext_size);

            if (csview_ends_with(no_ext, "."))
            {
                state->format = FORMAT_LOOKUP[i].format;
            }
        }
    }

    cstr_drop(&output_lower);

    if (state->format == FORMAT_UNKNOWN)
    {
        printf("error: unknown output format!!\n");
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

    if (!vm)
    {
        return EXIT_FAILURE;
    }

    IMC_VM_run_src_file(vm, cstr_str(&state.input_file));

    switch (state.format)
    {
        case FORMAT_PNG:
            IMC_VM_write_png(vm, cstr_str(&state.output_file));
            break;
        case FORMAT_JPG:
            IMC_VM_write_jpg(vm, cstr_str(&state.output_file));
            break;
        case FORMAT_BMP:
            IMC_VM_write_bmp(vm, cstr_str(&state.output_file));
            break;
        case FORMAT_TGA:
            IMC_VM_write_tga(vm, cstr_str(&state.output_file));
            break;
        case FORMAT_XPM:
            IMC_VM_write_xpm(vm, cstr_str(&state.output_file));
            break;
        default:
    }

    cstr_drop(&state.input_file);
    cstr_drop(&state.output_file);

    IMC_VM_free(vm);
    return EXIT_SUCCESS;
}
