#	SPDX-License-Identifier: GPL-2.0-only
#	lua-kconfig
#	Copyright (c) 2024 Yao Zi. All rights reserved.

LUA_PKGNAME		= lua5.4

LUA_CFLAGS		:= $(shell pkg-config --cflags $(LUA_PKGNAME))

CC			:= cc
CCLD			:= $(CC)
FLEX			:= flex
BISON			:= bison

CFLAGS			:=
LDFLAGS			:=

MODULE			:= kconfig.so
MODULE_OBJS		:= src/conf.o src/confdata.o src/expr.o src/preprocess.o
MODULE_OBJS		+= src/menu.o src/symbol.o src/util.o src/lua-kconfig.o
MODULE_OBJS		+= src/lexer.o src/parser.tab.o

$(MODULE): $(MODULE_OBJS)
	$(CCLD) $(MODULE_OBJS) -o $(MODULE) -shared $(LDFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ -fPIC $(LUA_CFLAGS) $(CFLAGS) -Iinclude -DYYDEBUG

src/lexer.c: src/lexer.l src/parser.tab.c
	$(FLEX) -o $@ $<

src/parser.tab.c: src/parser.y
	$(BISON) -o $@ --header=src/parser.tab.h $<

clean:
	-rm $(MODULE_OBJS)
	-rm src/lexer.c src/parser.tab.c src/parser.tab.h
