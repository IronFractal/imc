#include "xpm.h"

#include <stdio.h>
#include <stdint.h>

const char VALID_KEY_CHARS[] = "!#$%&'()*+,-./0123456789:;<=>?@"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`"
                               "abcdefghijklmnopqrstuvwxyz{|}~";
#define VALID_KEY_CHARS_LEN (sizeof(VALID_KEY_CHARS) - 1)

struct color
{
    union
    {
        struct
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        };
        uint32_t full;
    };
};

struct color_palette
{
    bool define_none;
    size_t size;
    struct color colors[VALID_KEY_CHARS_LEN * 2];
};

#define CONSTRAIN_COLOR(col) do { if (col.a == 0) { cur.full = 0; } else { col.a = 255; } } while (false)

bool palettize(struct color_palette *pal, int width, int height, const void *data)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            bool found = false;
            struct color cur = ((const struct color *)data)[y*width+x];

            CONSTRAIN_COLOR(cur);

            if (cur.full == 0)
            {
                pal->define_none = true;
                continue;
            }

            for (size_t i = 0; !found && i < pal->size; i++)
            {
                if (cur.full == pal->colors[i].full)
                {
                    found = true;
                }
            }

            if (!found && pal->size >= (VALID_KEY_CHARS_LEN * 2))
            {
                return false;
            }
            else if (!found)
            {
                pal->colors[pal->size] = cur;
                pal->size++;
            }
        }
    }

    return true;
}

bool IMC_write_xpm(const char *filename, int width, int height, const void *data)
{
    bool doublekey = false;
    struct color_palette palette = {};
    FILE *out = fopen(filename, "w");

    if (!out)
    {
        goto handle_failure;
    }

    if (!palettize(&palette, width, height, data))
    {
        goto handle_failure;
    }

    if (palette.size >= VALID_KEY_CHARS_LEN)
    {
        doublekey = true;
    }

    if (!fprintf(out, "/* XPM */\n"))
    {
        goto handle_failure;
    }

    if (!fprintf(out, "static char *test[] = {\n"))
    {
        goto handle_failure;
    }

    if (!fprintf(out, "    \"%d %d %lu 1\",\n", width, height, palette.size + (palette.define_none ? 1 : 0)))
    {
        goto handle_failure;
    }

    if (palette.define_none && !fprintf(out, "%s    \"  c None\",\n", doublekey ? " " : ""))
    {
        goto handle_failure;
    }

    for (size_t i = 0; i < palette.size; i++)
    {
        const char k1 = VALID_KEY_CHARS[i/(VALID_KEY_CHARS_LEN * 2)];
        const char k2 = VALID_KEY_CHARS[i%(VALID_KEY_CHARS_LEN * 2)];
        struct color *cur = &palette.colors[i];

        if (doublekey && !fprintf(out, "    \"%c%c c #%02X%02X%02X\",\n", k1, k2, cur->r, cur->g, cur->b))
        {
            goto handle_failure;
        }

        if (!doublekey && !fprintf(out, "    \"%c c #%02X%02X%02X\",\n", k2, cur->r, cur->g, cur->b))
        {
            goto handle_failure;
        }
    }

    for (int y = 0; y < height; y++)
    {
        if (!fprintf(out, "    \""))
        {
            goto handle_failure;
        }

        for (int x = 0; x < width; x++)
        {
            size_t id = 0;
            struct color cur = ((const struct color *)data)[y*width+x];

            CONSTRAIN_COLOR(cur);

            for (size_t i = 0; !id && i < palette.size; i++)
            {
                if (cur.full == palette.colors[i].full)
                {
                    id = i;
                }
            }

            if (doublekey)
            {
                const char k1 = VALID_KEY_CHARS[id/(VALID_KEY_CHARS_LEN * 2)];
                const char k2 = VALID_KEY_CHARS[id%(VALID_KEY_CHARS_LEN * 2)];

                if (!cur.full && !fprintf(out, "  "))
                {
                    goto handle_failure;
                }
                else if (cur.full && !fprintf(out, "%c%c", k1, k2))
                {
                    goto handle_failure;
                }
            }
            else
            {
                const char k = VALID_KEY_CHARS[id];

                if (!cur.full && !fprintf(out, " "))
                {
                    goto handle_failure;
                }
                else if (cur.full && !fprintf(out, "%c", k))
                {
                    goto handle_failure;
                }
            }
        }

        if (!fprintf(out, "\",\n"))
        {
            goto handle_failure;
        }
    }

    if (!fprintf(out, "};\n"))
    {
        goto handle_failure;
    }

    fclose(out);
    return true;
handle_failure:
    fclose(out);
    return false;
}
