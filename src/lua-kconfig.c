#include <lua.h>
#include <lauxlib.h>

#include "lkc_proto.h"
#include "internal.h"
#include "preprocess.h"

static int
lk_setenv(lua_State *l)
{
	const char *k = luaL_checkstring(l, 1);
	const char *v = luaL_checkstring(l, 2);
	env_add(k, v);
	return 0;
}

static void
convert_to_lua(lua_State *l)
{
	struct symbol *s;

	lua_newtable(l);

	for_all_symbols(s) {
		if (!s->name)
			continue;

		lua_newtable(l);
		lua_pushstring(l, s->name);
		lua_setfield(l, -2, "name");
		lua_setfield(l, -2, s->name);
	}
}

static int
lk_parse(lua_State *l)
{
	const char *p = luaL_checkstring(l, 1);
	conf_parse(p);

	convert_to_lua(l);

	return 1;
}

static luaL_Reg funcs[] = {
	{ "setenv", lk_setenv },
	{ "parse", lk_parse },
	{ NULL, NULL }
};

int
luaopen_kconfig(lua_State *l)
{
	luaL_newlib(l, funcs);
	return 1;
}
