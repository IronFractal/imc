#include "parse_stream.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

struct parse_stream
{
    int line;
    int column;
    int position;
    FILE *file;
};

struct parse_stream *ps_new(const char *filename)
{
    struct parse_stream *result = malloc(sizeof(struct parse_stream));

    if (!result)
    {
        goto failure;
    }

    result->line = 0;
    result->column = 0;
    result->position = 0;

    result->file = fopen(filename, "r");

    if (!result->file)
    {
        goto failure;
    }

    return result;
failure:
    ps_free(result);
    return nullptr;
}

bool ps_is_eof(struct parse_stream *pstream)
{
    if (!pstream || ferror(pstream->file) || feof(pstream->file))
    {
        return true;
    }

    return false;
}

char ps_pop(struct parse_stream *pstream)
{
    assert(pstream);

    int result = fgetc(pstream->file);

    if (result == EOF)
    {
        return '\0';
    }

    pstream->position++;

    if ((char)result == '\n')
    {
        pstream->line++;
        pstream->column = 0;
    }
    else
    {
        pstream->column++;
    }

    return (char)result;
}

char ps_peek(struct parse_stream *pstream)
{
    assert(pstream);

    int result = fgetc(pstream->file);

    if (result == EOF)
    {
        return '\0';
    }

    fseek(pstream->file, -1, SEEK_CUR);

    return (char)result;
}

int ps_position(struct parse_stream *pstream)
{
    assert(pstream);

    return pstream->position + 1;
}

int ps_column(struct parse_stream *pstream)
{
    assert(pstream);

    return pstream->column + 1;
}

int ps_line(struct parse_stream *pstream)
{
    assert(pstream);

    return pstream->line + 1;
}

void ps_free(struct parse_stream *pstream)
{
    if (!pstream)
    {
        return;
    }

    fclose(pstream->file);
    free(pstream);
}
