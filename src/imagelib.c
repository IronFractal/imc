#include "imagelib.h"

#include <stdlib.h>

#include <lauxlib.h>
#include <plutovg.h>
#include <stb_image_write.h>

#include "xpm.h"

#define SET_LUA_ERR(MSG) \
luaL_error(L, MSG); \
lua_error(L)

#define GET_IMG_STATE(L, NAME) \
struct imc_image_lib_state *NAME = lua_touserdata(L, lua_upvalueindex(1));  \
if (!NAME)                                                                  \
{                                                                           \
    SET_LUA_ERR("failed to get internal state");                            \
    return 0;                                                               \
}

#define INIT_IMG_STATE(NAME)                                                \
if (!img_init_check(NAME))                                                  \
{                                                                           \
    SET_LUA_ERR("failed to initialize internal state");                     \
    return 0;                                                               \
}

#define CONSTRAIN(VAR, MIN, MAX) (VAR < MIN ? MIN : (VAR > MAX ? MAX : VAR))

enum color_mode
{
    COLOR_MODE_RGB,
    COLOR_MODE_HSB,
};

struct imc_image_lib_state
{
    bool fill;
    bool stroke;
    bool initialized;

    float stroke_weight;
    plutovg_line_cap_t stroke_cap;

    enum color_mode color_mode;

    plutovg_color_t fill_color;
    plutovg_color_t stroke_color;

    plutovg_surface_t *surface;
    plutovg_canvas_t *canvas;
    plutovg_font_face_cache_t *font_cache;
};

static bool img_init(struct imc_image_lib_state *ims, int width, int height)
{
    const plutovg_color_t default_bg = PLUTOVG_MAKE_COLOR(0, 0, 0, 0);

    plutovg_canvas_destroy(ims->canvas);
    ims->canvas = nullptr;

    if (ims->surface && plutovg_surface_get_width(ims->surface) != width)
    {
        plutovg_surface_destroy(ims->surface);
        ims->surface = nullptr;
    }
    else if (ims->surface && plutovg_surface_get_height(ims->surface) != height)
    {
        plutovg_surface_destroy(ims->surface);
        ims->surface = nullptr;
    }

    if (!ims->surface)
    {
        ims->surface = plutovg_surface_create(width, height);
    }

    if (!ims->surface)
    {
        return false;
    }

    plutovg_surface_clear(ims->surface, &default_bg);

    ims->canvas = plutovg_canvas_create(ims->surface);

    if (!ims->canvas)
    {
        return false;
    }

    ims->font_cache = plutovg_font_face_cache_create();

    if (!ims->font_cache)
    {
        return false;
    }

    plutovg_font_face_cache_load_sys(ims->font_cache);

    plutovg_canvas_set_font_face_cache(ims->canvas, ims->font_cache);

    ims->initialized = true;

    return true;
}

static inline bool img_init_default(struct imc_image_lib_state *ims)
{
    return img_init(ims, 512, 512);
}

static inline bool img_init_check(struct imc_image_lib_state *ims)
{
    return ims->initialized || img_init_default(ims);
}

static int img_create(lua_State *L)
{
    GET_IMG_STATE(L, ims);

    lua_Integer width = luaL_optint(L, 1, 512);
    lua_Integer height = luaL_optint(L, 2, 512);

    if (!img_init(ims, width, height))
    {
        SET_LUA_ERR("failed to create image");
    }

    return 0;
}

static int img_get_width(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    lua_pushinteger(L, plutovg_surface_get_width(ims->surface));

    return 1;
}

static int img_get_height(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    lua_pushinteger(L, plutovg_surface_get_height(ims->surface));

    return 1;
}

static int img_state_save(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    plutovg_canvas_save(ims->canvas);

    return 0;
}

static int img_state_restore(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    plutovg_canvas_restore(ims->canvas);

    return 0;
}

static int img_mode_set_rgb(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    ims->color_mode = COLOR_MODE_RGB;

    return 0;
}

static int img_mode_set_hsb(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    ims->color_mode = COLOR_MODE_HSB;

    return 0;
}

static int img_background(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    plutovg_color_t color;

    if (ims->color_mode == COLOR_MODE_RGB)
    {
        int r = luaL_checkint(L, 1);
        int g = luaL_checkint(L, 2);
        int b = luaL_checkint(L, 3);
        int a = luaL_optint(L, 4, 255);

        r = CONSTRAIN(r, 0, 255);
        g = CONSTRAIN(g, 0, 255);
        b = CONSTRAIN(b, 0, 255);
        a = CONSTRAIN(a, 0, 255);

        plutovg_color_init_rgba8(&color, r, g, b, a);
    }
    else
    {
        float h = luaL_checknumber(L, 1);
        float s = luaL_checknumber(L, 2);
        float l = luaL_checknumber(L, 3);
        float a = luaL_optnumber(L, 4, 1.00);

        h = CONSTRAIN(h, 0, 360);
        s = CONSTRAIN(s, 0.00, 1.00);
        l = CONSTRAIN(l, 0.00, 1.00);
        a = CONSTRAIN(a, 0.00, 1.00);

        plutovg_color_init_hsla(&color, h, s, l, a);
    }

    plutovg_surface_clear(ims->surface, &color);

    return 0;
}

static int img_fill(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);
    plutovg_color_t color;

    if (ims->color_mode == COLOR_MODE_RGB)
    {
        int r = luaL_checkint(L, 1);
        int g = luaL_checkint(L, 2);
        int b = luaL_checkint(L, 3);
        int a = luaL_optint(L, 4, 255);

        r = CONSTRAIN(r, 0, 255);
        g = CONSTRAIN(g, 0, 255);
        b = CONSTRAIN(b, 0, 255);
        a = CONSTRAIN(a, 0, 255);

        plutovg_color_init_rgba8(&color, r, g, b, a);
    }
    else
    {
        float h = luaL_checknumber(L, 1);
        float s = luaL_checknumber(L, 2);
        float l = luaL_checknumber(L, 3);
        float a = luaL_optnumber(L, 4, 1.00);

        h = CONSTRAIN(h, 0, 360);
        s = CONSTRAIN(s, 0.00, 1.00);
        l = CONSTRAIN(l, 0.00, 1.00);
        a = CONSTRAIN(a, 0.00, 1.00);

        plutovg_color_init_hsla(&color, h, s, l, a);
    }

    ims->fill = true;
    ims->fill_color = color;

    return 0;
}

static int img_no_fill(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    ims->fill = false;

    return 0;
}

static int img_stroke(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);
    plutovg_color_t color;

    if (ims->color_mode == COLOR_MODE_RGB)
    {
        int r = luaL_checkint(L, 1);
        int g = luaL_checkint(L, 2);
        int b = luaL_checkint(L, 3);
        int a = luaL_optint(L, 4, 255);

        r = CONSTRAIN(r, 0, 255);
        g = CONSTRAIN(g, 0, 255);
        b = CONSTRAIN(b, 0, 255);
        a = CONSTRAIN(a, 0, 255);

        plutovg_color_init_rgba8(&color, r, g, b, a);
    }
    else
    {
        float h = luaL_checknumber(L, 1);
        float s = luaL_checknumber(L, 2);
        float l = luaL_checknumber(L, 3);
        float a = luaL_optnumber(L, 4, 1.00);

        h = CONSTRAIN(h, 0, 360);
        s = CONSTRAIN(s, 0.00, 1.00);
        l = CONSTRAIN(l, 0.00, 1.00);
        a = CONSTRAIN(a, 0.00, 1.00);

        plutovg_color_init_hsla(&color, h, s, l, a);
    }

    ims->stroke = true;
    ims->stroke_color = color;

    return 0;
}

static int img_no_stroke(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    ims->stroke = false;

    return 0;
}

static int img_stroke_weight(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    ims->stroke_weight = luaL_optnumber(L, 1, 1.00);

    return 0;
}

static int img_stroke_cap(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    const char *list[] =
    {
        [PLUTOVG_LINE_CAP_BUTT]     = "project",
        [PLUTOVG_LINE_CAP_ROUND]    = "round",
        [PLUTOVG_LINE_CAP_SQUARE]   = "square",
        nullptr,
    };

    ims->stroke_cap = luaL_checkoption(L, 1, "round", list);

    return 0;
}

static int img_set_compositing_mode(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    const char *list[] =
    {
        [PLUTOVG_OPERATOR_CLEAR]    = "clear",
        [PLUTOVG_OPERATOR_SRC]      = "src",
        [PLUTOVG_OPERATOR_DST]      = "dst",
        [PLUTOVG_OPERATOR_SRC_OVER] = "src_over",
        [PLUTOVG_OPERATOR_DST_OVER] = "dst_over",
        [PLUTOVG_OPERATOR_SRC_IN]   = "src_in",
        [PLUTOVG_OPERATOR_DST_IN]   = "dst_in",
        [PLUTOVG_OPERATOR_SRC_OUT]  = "src_out",
        [PLUTOVG_OPERATOR_DST_OUT]  = "dst_out",
        [PLUTOVG_OPERATOR_SRC_ATOP] = "src_atop",
        [PLUTOVG_OPERATOR_DST_ATOP] = "dst_atop",
        [PLUTOVG_OPERATOR_XOR]      = "xor",
        nullptr,
    };

    plutovg_canvas_set_operator(ims->canvas, luaL_checkoption(L, 1, "src_over", list));

    return 0;
}

static int img_circle(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float r = luaL_checknumber(L, 3);

    if (ims->fill)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->fill_color);
        plutovg_canvas_circle(ims->canvas, x, y, r);
        plutovg_canvas_fill(ims->canvas);
    }

    if (ims->stroke)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->stroke_color);
        plutovg_canvas_set_line_width(ims->canvas, ims->stroke_weight);
        plutovg_canvas_set_line_cap(ims->canvas, ims->stroke_cap);
        plutovg_canvas_circle(ims->canvas, x, y, r);
        plutovg_canvas_stroke(ims->canvas);
    }

    return 0;
}

static int img_ellipse(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float w = luaL_checknumber(L, 3) / 2.00;
    float h = luaL_checknumber(L, 4) / 2.00;

    if (ims->fill)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->fill_color);
        plutovg_canvas_ellipse(ims->canvas, x, y, w, h);
        plutovg_canvas_fill(ims->canvas);
    }

    if (ims->stroke)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->stroke_color);
        plutovg_canvas_set_line_width(ims->canvas, ims->stroke_weight);
        plutovg_canvas_set_line_cap(ims->canvas, ims->stroke_cap);
        plutovg_canvas_ellipse(ims->canvas, x, y, w, h);
        plutovg_canvas_stroke(ims->canvas);
    }

    return 0;
}

static int img_line(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    float x2 = luaL_checknumber(L, 3);
    float y2 = luaL_checknumber(L, 4);

    if (ims->stroke)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->stroke_color);
        plutovg_canvas_set_line_width(ims->canvas, ims->stroke_weight);
        plutovg_canvas_set_line_cap(ims->canvas, ims->stroke_cap);
        plutovg_canvas_move_to(ims->canvas, x1, y1);
        plutovg_canvas_line_to(ims->canvas, x2, y2);
        plutovg_canvas_stroke(ims->canvas);
    }

    return 0;
}

static int img_point(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);

    if (ims->stroke)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->stroke_color);
        plutovg_canvas_set_line_width(ims->canvas, 1.00);
        plutovg_canvas_rect(ims->canvas, x, y, 1.00, 1.00);
        plutovg_canvas_stroke(ims->canvas);
    }

    return 0;
}

static int img_quad(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    float x2 = luaL_checknumber(L, 3);
    float y2 = luaL_checknumber(L, 4);
    float x3 = luaL_checknumber(L, 5);
    float y3 = luaL_checknumber(L, 6);
    float x4 = luaL_checknumber(L, 7);
    float y4 = luaL_checknumber(L, 8);

    if (ims->fill)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->fill_color);
        plutovg_canvas_move_to(ims->canvas, x1, y1);
        plutovg_canvas_line_to(ims->canvas, x2, y2);
        plutovg_canvas_line_to(ims->canvas, x3, y3);
        plutovg_canvas_line_to(ims->canvas, x4, y4);
        plutovg_canvas_line_to(ims->canvas, x1, y1);
        plutovg_canvas_fill(ims->canvas);
    }

    if (ims->stroke)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->stroke_color);
        plutovg_canvas_set_line_width(ims->canvas, ims->stroke_weight);
        plutovg_canvas_set_line_cap(ims->canvas, ims->stroke_cap);
        plutovg_canvas_move_to(ims->canvas, x1, y1);
        plutovg_canvas_line_to(ims->canvas, x2, y2);
        plutovg_canvas_line_to(ims->canvas, x3, y3);
        plutovg_canvas_line_to(ims->canvas, x4, y4);
        plutovg_canvas_line_to(ims->canvas, x1, y1);
        plutovg_canvas_stroke(ims->canvas);
    }

    return 0;
}

static int img_rect(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);
    const int nargs = lua_gettop(L);

    float a = luaL_checknumber(L, 1);
    float b = luaL_checknumber(L, 2);
    float c = luaL_checknumber(L, 3);
    float d = luaL_checknumber(L, 4);
    float rx = 0.00;
    float ry = 0.00;

    if (nargs > 5)
    {
        rx = luaL_checknumber(L, 5);
        ry = luaL_checknumber(L, 6);
    }
    else if (nargs > 4)
    {
        rx = luaL_checknumber(L, 5);
        ry = luaL_checknumber(L, 5);
    }

    if (ims->fill)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->fill_color);
        plutovg_canvas_round_rect(ims->canvas, a, b, c, d, rx, ry);
        plutovg_canvas_fill(ims->canvas);
    }

    if (ims->stroke)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->stroke_color);
        plutovg_canvas_set_line_width(ims->canvas, ims->stroke_weight);
        plutovg_canvas_set_line_cap(ims->canvas, ims->stroke_cap);
        plutovg_canvas_round_rect(ims->canvas, a, b, c, d, rx, ry);
        plutovg_canvas_stroke(ims->canvas);
    }

    return 0;
}

static int img_square(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float s = luaL_checknumber(L, 3);

    if (ims->fill)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->fill_color);
        plutovg_canvas_rect(ims->canvas, x, y, s, s);
        plutovg_canvas_fill(ims->canvas);
    }

    if (ims->stroke)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->stroke_color);
        plutovg_canvas_set_line_width(ims->canvas, ims->stroke_weight);
        plutovg_canvas_set_line_cap(ims->canvas, ims->stroke_cap);
        plutovg_canvas_rect(ims->canvas, x, y, s, s);
        plutovg_canvas_stroke(ims->canvas);
    }

    return 0;
}

static int img_triangle(lua_State *L)
{
    GET_IMG_STATE(L, ims);
    INIT_IMG_STATE(ims);

    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    float x2 = luaL_checknumber(L, 3);
    float y2 = luaL_checknumber(L, 4);
    float x3 = luaL_checknumber(L, 5);
    float y3 = luaL_checknumber(L, 6);

    if (ims->fill)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->fill_color);
        plutovg_canvas_move_to(ims->canvas, x1, y1);
        plutovg_canvas_line_to(ims->canvas, x2, y2);
        plutovg_canvas_line_to(ims->canvas, x3, y3);
        plutovg_canvas_line_to(ims->canvas, x1, y1);
        plutovg_canvas_fill(ims->canvas);
    }

    if (ims->stroke)
    {
        plutovg_canvas_set_color(ims->canvas, &ims->stroke_color);
        plutovg_canvas_set_line_width(ims->canvas, ims->stroke_weight);
        plutovg_canvas_set_line_cap(ims->canvas, ims->stroke_cap);
        plutovg_canvas_move_to(ims->canvas, x1, y1);
        plutovg_canvas_line_to(ims->canvas, x2, y2);
        plutovg_canvas_line_to(ims->canvas, x3, y3);
        plutovg_canvas_line_to(ims->canvas, x1, y1);
        plutovg_canvas_stroke(ims->canvas);
    }

    return 0;
}

// static int img_text(lua_State *L)
// {
//     GET_IMG_STATE(L, ims);
//     INIT_IMG_STATE(ims);

//     const char *text = luaL_checkstring(L, 1);
//     float x = luaL_checknumber(L, 2);
//     float y = luaL_checknumber(L, 3);

//     // plutovg_canvas_select_font_face(ims->canvas, "FiraCode Nerd Font", false, false);
//     plutovg_canvas_select_font_face(ims->canvas, "Georgia", false, false);
//     plutovg_canvas_set_font_size(ims->canvas, 24);

//     if (ims->fill)
//     {
//         plutovg_canvas_set_color(ims->canvas, &ims->fill_color);
//         plutovg_canvas_add_text(ims->canvas, text, -1, PLUTOVG_TEXT_ENCODING_UTF8, x, y);
//         plutovg_canvas_fill(ims->canvas);
//     }

//     if (ims->stroke)
//     {
//         plutovg_canvas_set_color(ims->canvas, &ims->stroke_color);
//         plutovg_canvas_set_line_width(ims->canvas, ims->stroke_weight);
//         plutovg_canvas_set_line_cap(ims->canvas, ims->stroke_cap);
//         plutovg_canvas_add_text(ims->canvas, text, -1, PLUTOVG_TEXT_ENCODING_UTF8, x, y);
//         plutovg_canvas_stroke(ims->canvas);
//     }

//     return 0;
// }

static void register_func(lua_State *L, struct imc_image_lib_state *state, const char *name, lua_CFunction func)
{
    lua_pushlightuserdata(L, state);
    lua_pushcclosure(L, func, 1);
    lua_setfield(L, -2, name);
}

struct imc_image_lib_state *IMC_IMG_load(lua_State *state)
{
    struct imc_image_lib_state *res = calloc(1, sizeof(struct imc_image_lib_state));

    if (!state || !res)
    {
        return nullptr;
    }

    res->fill = true;
    res->stroke = true;

    res->stroke_cap = PLUTOVG_LINE_CAP_ROUND;
    res->stroke_weight = 1.00;

    res->fill_color = PLUTOVG_MAKE_COLOR(1, 1, 1, 1);
    res->stroke_color = PLUTOVG_MAKE_COLOR(0, 0, 0, 1);

    #define REGISTER_FN(NAME) register_func(state, res, #NAME, img_##NAME)

    lua_newtable(state);

    REGISTER_FN(create);
    REGISTER_FN(get_width);
    REGISTER_FN(get_height);
    REGISTER_FN(state_save);
    REGISTER_FN(state_restore);
    REGISTER_FN(mode_set_rgb);
    REGISTER_FN(mode_set_hsb);
    REGISTER_FN(background);
    REGISTER_FN(fill);
    REGISTER_FN(no_fill);
    REGISTER_FN(stroke);
    REGISTER_FN(no_stroke);
    REGISTER_FN(stroke_weight);
    REGISTER_FN(stroke_cap);
    REGISTER_FN(set_compositing_mode);
    REGISTER_FN(circle);
    REGISTER_FN(ellipse);
    REGISTER_FN(line);
    REGISTER_FN(point);
    REGISTER_FN(quad);
    REGISTER_FN(rect);
    REGISTER_FN(square);
    REGISTER_FN(triangle);
    // REGISTER_FN(text);

    lua_setglobal(state, "Image");

    return res;
}

bool IMC_IMG_write_png(struct imc_image_lib_state *state, const char *filename)
{
    if (!state || !filename)
    {
        return false;
    }

    if (!img_init_check(state))
    {
        return false;
    }

    if (!plutovg_surface_write_to_png(state->surface, filename))
    {
        return false;
    }

    return true;
}

bool IMC_IMG_write_jpg(struct imc_image_lib_state *state, const char *filename)
{
    if (!state || !filename)
    {
        return false;
    }

    if (!img_init_check(state))
    {
        return false;
    }

    if (!plutovg_surface_write_to_jpg(state->surface, filename, 100))
    {
        return false;
    }

    return true;
}

bool IMC_IMG_write_bmp(struct imc_image_lib_state *state, const char *filename)
{
    int width;
    int height;
    int stride;
    unsigned char *data;

    if (!state || !filename)
    {
        return false;
    }

    if (!img_init_check(state))
    {
        return false;
    }

    width = plutovg_surface_get_width(state->surface);
    height = plutovg_surface_get_height(state->surface);
    stride = plutovg_surface_get_stride(state->surface);
    data = plutovg_surface_get_data(state->surface);

    plutovg_convert_argb_to_rgba(data, data, width, height, stride);

    stbi_write_bmp(filename, width, height, 4, data);

    plutovg_convert_rgba_to_argb(data, data, width, height, stride);

    return true;
}

bool IMC_IMG_write_tga(struct imc_image_lib_state *state, const char *filename)
{
    int width;
    int height;
    int stride;
    unsigned char *data;

    if (!state || !filename)
    {
        return false;
    }

    if (!img_init_check(state))
    {
        return false;
    }

    width = plutovg_surface_get_width(state->surface);
    height = plutovg_surface_get_height(state->surface);
    stride = plutovg_surface_get_stride(state->surface);
    data = plutovg_surface_get_data(state->surface);

    plutovg_convert_argb_to_rgba(data, data, width, height, stride);

    stbi_write_tga(filename, width, height, 4, data);

    plutovg_convert_rgba_to_argb(data, data, width, height, stride);

    return true;
}

bool IMC_IMG_write_xpm(struct imc_image_lib_state *state, const char *filename)
{
    int width;
    int height;
    int stride;
    unsigned char *data;

    if (!state || !filename)
    {
        return false;
    }

    if (!img_init_check(state))
    {
        return false;
    }

    width = plutovg_surface_get_width(state->surface);
    height = plutovg_surface_get_height(state->surface);
    stride = plutovg_surface_get_stride(state->surface);
    data = plutovg_surface_get_data(state->surface);

    plutovg_convert_argb_to_rgba(data, data, width, height, stride);

    IMC_write_xpm(filename, width, height, data);

    plutovg_convert_rgba_to_argb(data, data, width, height, stride);

    return true;
}

void IMC_IMG_free(struct imc_image_lib_state *state)
{
    if (!state)
    {
        return;
    }

    plutovg_canvas_destroy(state->canvas);
    plutovg_surface_destroy(state->surface);
    plutovg_font_face_cache_destroy(state->font_cache);
    free(state);
}
