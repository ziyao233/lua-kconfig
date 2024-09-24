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

#define str_field(l, s, name) do { \
	lua_pushstring(l, s->name);				\
	lua_setfield(l, -2, #name);				\
} while (0)

static
void add_const(lua_State *l, struct symbol *s)
{
	if (!s->name || !(s->flags & SYMBOL_CONST))
		return;

	lua_newtable(l);
	str_field(l, s, name);

	lua_pushstring(l, sym_type_name(s->type));
	lua_setfield(l, -2, "type");

	lua_setfield(l, -2, s->name);
}

static int
convert_to_lua(lua_State *l)
{
	struct symbol *s;

	/* construct constants table */
	lua_newtable(l);
	for_all_symbols(s)
		add_const(l, s);
	add_const(l, &symbol_yes);
	add_const(l, &symbol_mod);
	add_const(l, &symbol_no);

	/* main symbol table */
	lua_newtable(l);
	for_all_symbols(s) {
		if (!s->name || (s->flags & SYMBOL_CONST))
			continue;

		lua_newtable(l);

		str_field(l, s, name);

		lua_pushstring(l, sym_type_name(s->type));
		lua_setfield(l, -2, "type");

		lua_newtable(l);
		for (struct property *p = s->prop; p; p = p->next) {
			lua_pushinteger(l, p->lineno);
			lua_setfield(l, -2, p->filename);
		}
		lua_setfield(l, -2, "location");

		lua_setfield(l, -2, s->name);
	}

	return 2;
}

static int
lk_parse(lua_State *l)
{
	const char *p = luaL_checkstring(l, 1);
	conf_parse(p);

	return convert_to_lua(l);
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
