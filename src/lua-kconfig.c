#include <assert.h>

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
	if (!(s->flags & SYMBOL_CONST))
		return;

	lua_newtable(l);

	lua_pushstring(l, sym_type_name(s->type));
	lua_setfield(l, -2, "type");

	lua_pushboolean(l, 1);
	lua_setfield(l, -2, "isConst");

	lua_pushstring(l, sym_get_string_value(s));
	lua_setfield(l, -2, "value");

	lua_rawsetp(l, -2, s);
}

static const char *
expr_type_name(enum expr_type t)
{
	static const char *exprNames[] = {
		[E_OR] = "or",
		[E_AND] = "and",
		[E_NOT] = "not",
		[E_EQUAL] = "equal",
		[E_UNEQUAL] = "unequal",
		[E_LTH] = "less",
		[E_LEQ] = "less-or-eq",
		[E_GTH] = "greater",
		[E_GEQ] = "greater-or-eq",
		[E_SYMBOL] = "symbol",
		[E_RANGE] = "range"
	};

	assert(t > E_NONE && t <= E_RANGE);

	return exprNames[t];
}

static inline int
is_compare(enum expr_type t)
{
	switch (t) {
	case E_EQUAL:
	case E_UNEQUAL:
	case E_GEQ:
	case E_GTH:
	case E_LEQ:
	case E_LTH:
		return 1;
	default:
		return 0;
	}
}

static inline void
search_symbol(lua_State *l, int constTblIdx, int symTblIdx, struct symbol *s)
{
	lua_rawgetp(l, s->flags & SYMBOL_CONST ? constTblIdx : symTblIdx, s);
	assert(lua_type(l, -1) == LUA_TTABLE);
}

static void
convert_expr(lua_State *l, int constTblIdx, int symTblIdx, struct expr *expr)
{
	luaL_checkstack(l, 2, NULL);

	lua_newtable(l);

	lua_pushstring(l, expr_type_name(expr->type));
	lua_setfield(l, -2, "fn");

	if (expr->type == E_SYMBOL) {
		struct symbol *s = expr->left.sym;
		search_symbol(l, constTblIdx, symTblIdx, s);
		lua_setfield(l, -2, "ref");
	} else if (is_compare(expr->type)) {
		struct symbol *left = expr->left.sym, *right = expr->right.sym;
		search_symbol(l, constTblIdx, symTblIdx, left);
		lua_setfield(l, -2, "left");
		search_symbol(l, constTblIdx, symTblIdx, left);
		lua_setfield(l, -2, "right");
	} else {
		struct expr *left = expr->left.expr, *right = expr->right.expr;
		convert_expr(l, constTblIdx, symTblIdx, left);
		lua_setfield(l, -2, "left");
		if (expr->type != E_NOT) {
			convert_expr(l, constTblIdx, symTblIdx, right);
			lua_setfield(l, -2, "right");
		}
	}
}

static int
convert_to_lua(lua_State *l)
{
	struct symbol *s;

	luaL_checkstack(l, 20, NULL);

	/* construct constants table, at 1 */
	lua_newtable(l);
	for_all_symbols(s)
		add_const(l, s);
	add_const(l, &symbol_yes);
	add_const(l, &symbol_mod);
	add_const(l, &symbol_no);

	lua_newtable(l);	// main symbol lookup table, at 2
	/* main symbol table, at 3 */
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

		lua_pushvalue(l, -1);
		lua_setfield(l, 3, s->name);
		lua_rawsetp(l, 2, s);
	}

	/* fix up dependencies */
	for_all_symbols(s) {
		if (!s->name || (s->flags & SYMBOL_CONST))
			continue;

		lua_rawgetp(l, 2, s);
		assert(lua_type(l, -1) == LUA_TTABLE);

		if (s->dir_dep.expr) {
			convert_expr(l, 1, 2, s->dir_dep.expr);
			lua_setfield(l, -2, "depends");
		}

		lua_newtable(l);
		struct property *p;
		int i = 1;
		for_all_properties(s, p, P_SELECT) {
			convert_expr(l, 1, 2, p->expr);
			lua_rawseti(l, -2, i);
			i++;
		}
		lua_setfield(l, -2, "select");

		lua_pop(l, 1);
	}

	return 1;	// return only the main symbol table
}

static int
lk_parse(lua_State *l)
{
	const char *p = luaL_checkstring(l, 1);
	conf_parse(p);
	lua_pop(l, 1);

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
