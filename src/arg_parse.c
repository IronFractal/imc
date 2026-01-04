#include "arg_parse.h"

#include <getopt.h>
#include <stdlib.h>

#define MIN(a, b) (a <= b ? a : b)

static int current_mutex_id = 0;

struct arg_mutex ARG_mutex_new()
{
    struct arg_mutex ret =
    {
        .id = ++current_mutex_id,
    };

    return ret;
}

static bool parse_internal(struct arg_parse argp, int argc, char **argv, bool allow_unknown)
{
    int c;
    int option_index = 0;
    size_t cur_long_opt = 0;
    size_t num_long_opts = 0;
    cstr short_opt_str = cstr_init();
    struct option *long_opts = nullptr;

    if (!cstr_append(&short_opt_str, ":"))
    {
        goto parse_failure;
    }

    if (argp.help)
    {
        if (!cstr_append(&short_opt_str, "h"))
        {
            goto parse_failure;
        }

        num_long_opts++;
    }

    for (int i = 0; argp.args[i].flag_val; i++)
    {
        if (argp.args[i].short_opt)
        {
            cstr_append_fmt(&short_opt_str, "%c", argp.args[i].short_opt);
        }

        if (argp.args[i].type == ARG_TYPE_ARG_OPTIONAL)
        {
            cstr_append(&short_opt_str, "::");
        }
        else if (argp.args[i].type == ARG_TYPE_ARG_REQUIRED)
        {
            cstr_append(&short_opt_str, ":");
        }

        if (argp.args[i].long_opt)
        {
            num_long_opts++;
        }
    }

    long_opts = calloc(num_long_opts+1, sizeof(struct option));

    if (!long_opts)
    {
        goto parse_failure;
    }

    if (argp.help)
    {
        long_opts[0].name = "help";
        long_opts[0].val = 'h';

        cur_long_opt++;
    }

    for (int i = 0; argp.args[i].flag_val; i++)
    {
        if (argp.args[i].long_opt)
        {
            long_opts[cur_long_opt].name = argp.args[i].long_opt;
            long_opts[cur_long_opt].val = argp.args[i].short_opt;
            long_opts[cur_long_opt].has_arg = no_argument;

            if (argp.args[i].type == ARG_TYPE_ARG_REQUIRED)
            {
                long_opts[cur_long_opt].has_arg = required_argument;
            }
            else if (argp.args[i].type == ARG_TYPE_ARG_OPTIONAL)
            {
                long_opts[cur_long_opt].has_arg = optional_argument;
            }

            cur_long_opt++;
        }
    }

    while ((c = getopt_long(argc, argv, cstr_str(&short_opt_str), long_opts, &option_index)) != -1)
    {
        //int cur_optind = optind ? optind : 1;

        switch (c)
        {
            case 0:
            case '0':
            case '1':
            case '2':
                printf("idk ?? %c\n", c);
                break;
            case 'h':
                if (argp.help)
                {
                    ARG_print_usage(argp);
                    exit(0);
                }
                else
                {
                    goto handle_arg;
                }
                break;
            case '?':
                if (!allow_unknown)
                {
                    if (optopt)
                    {
                        printf("error: unknown option '-%c'\n", optopt);
                    }
                    else
                    {
                        printf("error: unknown option '%s'\n", argv[optind - 1]);
                    }
                    ARG_print_usage(argp);
                    goto parse_failure;
                }
                break;
            case ':':
                printf("error: missing required argument for '-%c'\n", optopt);
                goto parse_failure;
            default:
handle_arg:
            {
                bool found = false;
                for (int i = 0; argp.args[i].flag_val; i++)
                {
                    if (argp.args[i].short_opt == c)
                    {
                        found = true;
                        if (argp.args[i].type == ARG_TYPE_FLAG)
                        {
                            if (argp.args[i].flag_val)
                            {
                                (*argp.args[i].flag_val) = true;
                            }
                        }
                        else
                        {
                            if (argp.args[i].string_val)
                            {
                                (*argp.args[i].string_val) = cstr_from(optarg);
                            }
                        }
                    }
                }
                if (!found && !allow_unknown)
                {
                    // todo
                    goto parse_failure;
                }
            }
        }

    }

    free(long_opts);
    cstr_drop(&short_opt_str);
    return true;
parse_failure:
    free(long_opts);
    cstr_drop(&short_opt_str);
    return false;
}

void ARG_print_usage(struct arg_parse argp)
{
    printf("Usage:\n  %s [options]\n\nOptions:\n", argp.name);

    if (argp.help)
    {
        printf("  -h, --help\n    Print out this help information.\n");
    }

    for (int i = 0; argp.args[i].flag_val; i++)
    {
        if (argp.args[i].short_opt && argp.args[i].long_opt)
        {
            printf("  -%c, --%s", argp.args[i].short_opt, argp.args[i].long_opt);
        }
        else if (argp.args[i].short_opt)
        {
            printf("  -%c", argp.args[i].short_opt);
        }
        else
        {
            printf("  --%s", argp.args[i].long_opt);
        }

        switch (argp.args[i].type)
        {
            case ARG_TYPE_FLAG:
                printf("\n");
                break;
            case ARG_TYPE_ARG_OPTIONAL:
                printf(" [argument]\n");
                break;
            case ARG_TYPE_ARG_REQUIRED:
                printf(" <argument>\n");
                break;
        }

        if (argp.args[i].description)
        {
            cstr desc = cstr_from(argp.args[i].description);

            for (long j = 0; j < cstr_size(&desc); j += 76)
            {
                csview segment = cstr_subview(&desc, j, MIN(76, cstr_size(&desc) - j));
                printf("    " c_svfmt "\n", c_svarg(segment));
            }

            cstr_drop(&desc);

            // const unsigned long desc_len = strlen(argp.args[i].description);
            // cstr cur_line = cstr_from_n()
            // str *cur_line = STR_new2(STR_GROW_MIN, 80);

            // for (unsigned long j = 0; j < desc_len; j += 76)
            // {
            //     STR_assign_cstr2(cur_line, argp.args[i].description + j, MIN(76, desc_len - j));
            //     printf("    %s\n", STR_cstr(cur_line));
            // }

            // STR_free(cur_line);
        }
    }
}

bool ARG_parse(struct arg_parse argp, int argc, char **argv)
{
    return parse_internal(argp, argc, argv, false);
}

bool ARG_parse_known(struct arg_parse argp, int argc, char **argv)
{
    return parse_internal(argp, argc, argv, true);
}
