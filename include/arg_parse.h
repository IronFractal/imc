#ifndef ARG_PARSE_H
#define ARG_PARSE_H
#include <stc/cstr.h>

enum arg_type
{
    ARG_TYPE_FLAG,
    ARG_TYPE_ARG_REQUIRED,
    ARG_TYPE_ARG_OPTIONAL,
};

struct arg_mutex
{
    int id;
};

struct arg_conf
{
    union
    {
        bool *flag_val;
        cstr *string_val;
    };
    const char short_opt;
    const char *long_opt;
    const char *description;
    const struct arg_mutex *mut;
    enum arg_type type;
};

struct arg_parse
{
    const char *name;
    struct arg_conf *args;
    bool help;
};

struct arg_mutex ARG_mutex_new();

void ARG_print_usage(struct arg_parse argp);

bool ARG_parse(struct arg_parse argp, int argc, char **argv);

bool ARG_parse_known(struct arg_parse argp, int argc, char **argv);

#endif
