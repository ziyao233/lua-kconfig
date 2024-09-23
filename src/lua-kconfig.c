#include <lua.h>
#include <lauxlib.h>

#include "lkc_proto.h"

static int
lk_parse(lua_State *l)
{
	const char *p = luaL_checkstring(l, 1);
	conf_parse(p);
	return 0;
}

static luaL_Reg funcs[] = {
	{ "parse", lk_parse },
	{ NULL, NULL }
};

int
luaopen_kconfig(lua_State *l)
{
	luaL_newlib(l, funcs);
	return 1;
}
