// SPDX-License-Identifier: LGPL-3.0-or-later
/*******************************************************************************
 * shadowblox - https://git.seki.pw/Fumohouse/shadowblox
 *
 * Copyright 2025-present ksk.
 * Copyright 2025-present shadowblox contributors.
 *
 * Licensed under the GNU Lesser General Public License version 3.0 or later.
 * See COPYRIGHT.txt for more details.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "Sbx/Runtime/Stack.hpp"

#include <cmath>
#include <string>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"

/* BASIC TYPES */

#define BASIC_STACK_OP_IMPL(mType, mOpName, mIsName)                                                                 \
	void LuauStackOp<mType>::push(lua_State *L, const mType &value) { lua_push##mOpName(L, value); }                 \
	mType LuauStackOp<mType>::get(lua_State *L, int index) { return static_cast<mType>(lua_to##mOpName(L, index)); } \
	bool LuauStackOp<mType>::is(lua_State *L, int index) { return lua_is##mIsName(L, index); }                       \
	mType LuauStackOp<mType>::check(lua_State *L, int index) { return static_cast<mType>(luaL_check##mOpName(L, index)); }

BASIC_STACK_OP_IMPL(bool, boolean, boolean);
BASIC_STACK_OP_IMPL(int, integer, number);
BASIC_STACK_OP_IMPL(float, number, number);
BASIC_STACK_OP_IMPL(double, number, number);
BASIC_STACK_OP_IMPL(int8_t, number, number);
BASIC_STACK_OP_IMPL(uint8_t, unsigned, number);
BASIC_STACK_OP_IMPL(int16_t, number, number);
BASIC_STACK_OP_IMPL(uint16_t, unsigned, number);
BASIC_STACK_OP_IMPL(uint32_t, unsigned, number);

/* 64-BIT INTEGER */

#define INT64_MT_NAME "Luau.Int64"
#define INT64_DOUBLE_LIMIT 9007199254740992 // 2^53; see https://stackoverflow.com/a/1848762

void LuauStackOp<int64_t>::initMetatable(lua_State *L) {
	luaL_newmetatable(L, INT64_MT_NAME);

	lua_pushstring(L, "Int64");
	lua_setfield(L, -2, "__type");

#define INT64_OP_FUNC(mExp, mName, mRet)                                                                 \
	lua_pushcfunction(                                                                                   \
			L, [](lua_State *L) {                                                                        \
				double d1 = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : LuauStackOp<int64_t>::check(L, 1); \
				double d2 = lua_isnumber(L, 2) ? lua_tonumber(L, 2) : LuauStackOp<int64_t>::check(L, 2); \
				LuauStackOp<mRet>::push(L, mExp);                                                        \
				return 1;                                                                                \
			},                                                                                           \
			INT64_MT_NAME "." mName);                                                                    \
                                                                                                         \
	lua_setfield(L, -2, mName);

	INT64_OP_FUNC(d1 + d2, "__add", double)
	INT64_OP_FUNC(d1 - d2, "__sub", double)
	INT64_OP_FUNC(d1 * d2, "__mul", double)
	INT64_OP_FUNC(d1 / d2, "__div", double)
	INT64_OP_FUNC(fmod(d1, d2), "__mod", double)
	INT64_OP_FUNC(pow(d1, d2), "__pow", double)

	INT64_OP_FUNC(d1 == d2, "__eq", bool)
	INT64_OP_FUNC(d1 < d2, "__lt", bool)

	// Special: negate
	lua_pushcfunction(
			L, [](lua_State *L) {
				LuauStackOp<int64_t>::push(L, -LuauStackOp<int64_t>::check(L, 1));
				return 1;
			},
			INT64_MT_NAME ".__unm");

	lua_setfield(L, -2, "__unm");

	// Special: stringify
	lua_pushcfunction(
			L, [](lua_State *L) {
				LuauStackOp<std::string>::push(L, std::to_string(LuauStackOp<int64_t>::check(L, 1)));
				return 1;
			},
			INT64_MT_NAME ".__tostring");

	lua_setfield(L, -2, "__tostring");

	lua_setreadonly(L, -1, true);
	lua_setuserdatametatable(L, Int64Udata);
}

void LuauStackOp<int64_t>::pushI64(lua_State *L, const int64_t &value) {
	int64_t *udata = reinterpret_cast<int64_t *>(lua_newuserdatataggedwithmetatable(L, sizeof(int64_t), Int64Udata));
	*udata = value;
}

void LuauStackOp<int64_t>::push(lua_State *L, const int64_t &value) {
	if (value >= -INT64_DOUBLE_LIMIT && value <= INT64_DOUBLE_LIMIT) {
		lua_pushnumber(L, value);
	} else {
		LuauStackOp<int64_t>::pushI64(L, value);
	}
}

int64_t LuauStackOp<int64_t>::get(lua_State *L, int index) {
	if (lua_isnumber(L, index))
		return lua_tonumber(L, index);

	int64_t *udata = reinterpret_cast<int64_t *>(lua_touserdatatagged(L, index, Int64Udata));
	if (udata)
		return *udata;

	return 0;
}

bool LuauStackOp<int64_t>::is(lua_State *L, int index) {
	return lua_isnumber(L, index) || lua_touserdatatagged(L, index, Int64Udata);
}

int64_t LuauStackOp<int64_t>::check(lua_State *L, int index) {
	int64_t *udata = reinterpret_cast<int64_t *>(lua_touserdatatagged(L, index, Int64Udata));
	if (udata)
		return *udata;

	return luaL_checknumber(L, index);
}

/* STRING */

void LuauStackOp<std::string>::push(lua_State *L, const std::string &value) {
	lua_pushstring(L, value.c_str());
}

std::string LuauStackOp<std::string>::get(lua_State *L, int index) {
	return lua_tostring(L, index);
}

bool LuauStackOp<std::string>::is(lua_State *L, int index) {
	return lua_isstring(L, index);
}

std::string LuauStackOp<std::string>::check(lua_State *L, int index) {
	return luaL_checkstring(L, index);
}

void LuauStackOp<const char *>::push(lua_State *L, const char *value) { lua_pushstring(L, value); }
const char *LuauStackOp<const char *>::get(lua_State *L, int index) { return lua_tostring(L, index); }
bool LuauStackOp<const char *>::is(lua_State *L, int index) { return lua_isstring(L, index); }
const char *LuauStackOp<const char *>::check(lua_State *L, int index) { return luaL_checkstring(L, index); }
