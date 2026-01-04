#include "langvm.h"

#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "imagelib.h"

struct imc_lang_vm
{
    lua_State *l_state;
    struct imc_image_lib_state *imgst;
};

struct imc_lang_vm *IMC_VM_new()
{
    struct imc_lang_vm *res = malloc(sizeof(struct imc_lang_vm));

    if (!res)
    {
        goto failure;
    }

    res->l_state = luaL_newstate();

    if (!res->l_state)
    {
        goto failure;
    }

    luaL_openlibs(res->l_state);

    res->imgst = IMC_IMG_load(res->l_state);

    if (!res->imgst)
    {
        goto failure;
    }

    return res;
failure:
    IMC_VM_free(res);
    return nullptr;
}

bool IMC_VM_run_src(struct imc_lang_vm *vm, const char *src)
{
    if (!vm || !src)
    {
        return false;
    }

    if (luaL_dostring(vm->l_state, src) != LUA_OK)
    {
        puts(lua_tostring(vm->l_state, lua_gettop(vm->l_state)));
        lua_pop(vm->l_state, lua_gettop(vm->l_state));
        return false;
    }

    return true;
}

bool IMC_VM_run_src_file(struct imc_lang_vm *vm, const char *filename)
{
    if (!vm || !filename)
    {
        return false;
    }

    if (luaL_dofile(vm->l_state, filename) != LUA_OK)
    {
        puts(lua_tostring(vm->l_state, lua_gettop(vm->l_state)));
        lua_pop(vm->l_state, lua_gettop(vm->l_state));
        return false;
    }

    return true;
}

inline bool IMC_VM_write_png(struct imc_lang_vm *vm, const char *filename)
{
    return IMC_IMG_write_png(vm->imgst, filename);
}

void IMC_VM_free(struct imc_lang_vm *vm)
{
    if (!vm)
    {
        return;
    }

    lua_close(vm->l_state);
    IMC_IMG_free(vm->imgst);
    free(vm);
}
