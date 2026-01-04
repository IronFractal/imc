#ifndef IMC_IMAGELIB_H
#define IMC_IMAGELIB_H
#include "lua.h"

struct imc_image_lib_state;

struct imc_image_lib_state *IMC_IMG_load(lua_State *state);

bool IMC_IMG_write_png(struct imc_image_lib_state *state, const char *filename);

void IMC_IMG_free(struct imc_image_lib_state *state);

#endif
