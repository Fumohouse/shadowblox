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

#pragma once

#include <string>

#include "lua.h"

template <typename T>
struct LuauStackOp {};

#define STACK_OP_DEF(mType)                                 \
	template <>                                             \
	struct LuauStackOp<mType> {                             \
		static void push(lua_State *L, const mType &value); \
                                                            \
		static mType get(lua_State *L, int index);          \
		static bool is(lua_State *L, int index);            \
		static mType check(lua_State *L, int index);        \
	};

#define STACK_OP_PTR_DEF(mType)                             \
	template <>                                             \
	struct LuauStackOp<mType> {                             \
		static void push(lua_State *L, const mType &value); \
                                                            \
		static mType get(lua_State *L, int index);          \
		static bool is(lua_State *L, int index);            \
		static mType check(lua_State *L, int index);        \
                                                            \
		/* USERDATA */                                      \
                                                            \
		static mType *alloc(lua_State *L);                  \
		static mType *getPtr(lua_State *L, int index);      \
		static mType *checkPtr(lua_State *L, int index);    \
	};

STACK_OP_DEF(bool)
STACK_OP_DEF(int)
STACK_OP_DEF(float)

STACK_OP_DEF(double)
STACK_OP_DEF(int8_t)
STACK_OP_DEF(uint8_t)
STACK_OP_DEF(int16_t)
STACK_OP_DEF(uint16_t)
STACK_OP_DEF(uint32_t)

template <>
struct LuauStackOp<int64_t> {
	static void initMetatable(lua_State *L);

	static void pushI64(lua_State *L, const int64_t &value);
	static void push(lua_State *L, const int64_t &value);

	static int64_t get(lua_State *L, int index);
	static bool is(lua_State *L, int index);
	static int64_t check(lua_State *L, int index);
};

STACK_OP_DEF(std::string)

template <>
struct LuauStackOp<const char *> {
	static void push(lua_State *L, const char *value);

	static const char *get(lua_State *L, int index);
	static bool is(lua_State *L, int index);
	static const char *check(lua_State *L, int index);
};

/* USERDATA */

#define UDATA_ALLOC(mType, mTag, mDtor)                                                                       \
	mType *LuauStackOp<mType>::alloc(lua_State *L) {                                                          \
		/* FIXME: Shouldn't happen every time, but probably is fast */                                        \
		lua_setuserdatadtor(L, mTag, mDtor);                                                                  \
                                                                                                              \
		mType *udata = reinterpret_cast<mType *>(lua_newuserdatataggedwithmetatable(L, sizeof(mType), mTag)); \
		new (udata) mType();                                                                                  \
                                                                                                              \
		return udata;                                                                                         \
	}

#define UDATA_GET_PTR(mType, mTag)                                              \
	mType *LuauStackOp<mType>::getPtr(lua_State *L, int index) {                \
		return reinterpret_cast<mType *>(lua_touserdatatagged(L, index, mTag)); \
	}

#define UDATA_CHECK_PTR(mType, mMetatableName, mTag)               \
	mType *LuauStackOp<mType>::checkPtr(lua_State *L, int index) { \
		void *udata = lua_touserdatatagged(L, index, mTag);        \
		if (!udata)                                                \
			luaL_typeerrorL(L, index, mMetatableName);             \
                                                                   \
		return reinterpret_cast<mType *>(udata);                   \
	}

#define UDATA_STACK_OP_IMPL(mType, mMetatableName, mTag, mDtor)       \
	UDATA_ALLOC(mType, mTag, mDtor)                                   \
                                                                      \
	void LuauStackOp<mType>::push(lua_State *L, const mType &value) { \
		mType *udata = LuauStackOp<mType>::alloc(L);                  \
		*udata = value;                                               \
	}                                                                 \
                                                                      \
	bool LuauStackOp<mType>::is(lua_State *L, int index) {            \
		return lua_touserdatatagged(L, index, mTag);                  \
	}                                                                 \
                                                                      \
	UDATA_GET_PTR(mType, mTag)                                        \
                                                                      \
	mType LuauStackOp<mType>::get(lua_State *L, int index) {          \
		mType *udata = LuauStackOp<mType>::getPtr(L, index);          \
		if (!udata)                                                   \
			return mType();                                           \
                                                                      \
		return *udata;                                                \
	}                                                                 \
                                                                      \
	UDATA_CHECK_PTR(mType, mMetatableName, mTag)                      \
                                                                      \
	mType LuauStackOp<mType>::check(lua_State *L, int index) {        \
		return *LuauStackOp<mType>::checkPtr(L, index);               \
	}

#define NO_DTOR [](lua_State *, void *) {}
#define DTOR(mType)                                 \
	[](lua_State *, void *udata) {                  \
		reinterpret_cast<mType *>(udata)->~mType(); \
	}
