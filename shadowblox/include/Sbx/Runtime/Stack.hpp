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

#include <cstdint>
#include <optional>
#include <string>

#include "lua.h"

namespace SBX {

template <typename T>
struct LuauStackOp {};

template <typename T>
struct LuauStackOp<const T *> {
	static void Push(lua_State *L, const T *value) {
		LuauStackOp<T *>::Push(L, value);
	}

	static const T *Get(lua_State *L, int index) {
		return LuauStackOp<T *>::Get(L, index);
	}

	static bool Is(lua_State *L, int index) {
		return LuauStackOp<T>::Is(L, index);
	}

	static const T *Check(lua_State *L, int index) {
		return LuauStackOp<T *>::Check(L, index);
	}
};

template <typename T>
struct LuauStackOp<T &> {
	static void Push(lua_State *L, const T &value) {
		LuauStackOp<T>::Push(L, value);
	}

	static T &Get(lua_State *L, int index) {
		return *LuauStackOp<T *>::Get(L, index);
	}

	static bool Is(lua_State *L, int index) {
		LuauStackOp<T>::Is(L, index);
	}

	static T &Check(lua_State *L, int index) {
		return *LuauStackOp<T *>::Check(L, index);
	}
};

template <typename T>
struct LuauStackOp<const T &> {
	static void Push(lua_State *L, const T &value) {
		LuauStackOp<T>::Push(L, value);
	}

	static const T &Get(lua_State *L, int index) {
		return *LuauStackOp<T *>::Get(L, index);
	}

	static bool Is(lua_State *L, int index) {
		LuauStackOp<T>::Is(L, index);
	}

	static const T &Check(lua_State *L, int index) {
		return *LuauStackOp<T *>::Check(L, index);
	}
};

template <typename T>
struct LuauStackOp<std::optional<T>> {
	static void Push(lua_State *L, const std::optional<T> &value) {
		if (value) {
			LuauStackOp<T>::Push(L, *value);
		} else {
			lua_pushnil(L);
		}
	}

	static std::optional<T> Get(lua_State *L, int index) {
		if (LuauStackOp<T>::Is(L, index)) {
			return LuauStackOp<T>::Get(L, index);
		} else {
			return std::nullopt;
		}
	}

	static bool Is(lua_State *L, int index) {
		return true;
	}

	static std::optional<T> Check(lua_State *L, int index) {
		return Get(L, index);
	}
};

#define STACK_OP_DEF(mType)                                 \
	template <>                                             \
	struct LuauStackOp<mType> {                             \
		static void Push(lua_State *L, const mType &value); \
                                                            \
		static mType Get(lua_State *L, int index);          \
		static bool Is(lua_State *L, int index);            \
		static mType Check(lua_State *L, int index);        \
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
	static void InitMetatable(lua_State *L);

	static void PushI64(lua_State *L, const int64_t &value);
	static void Push(lua_State *L, const int64_t &value);

	static int64_t Get(lua_State *L, int index);
	static bool Is(lua_State *L, int index);
	static int64_t Check(lua_State *L, int index);
};

STACK_OP_DEF(std::string)

template <>
struct LuauStackOp<const char *> {
	static void Push(lua_State *L, const char *value);

	static const char *Get(lua_State *L, int index);
	static bool Is(lua_State *L, int index);
	static const char *Check(lua_State *L, int index);
};

/* USERDATA: Use for (typ. immutable) objects owned by Luau */

#define STACK_OP_UDATA_DEF(mType)                           \
	template <>                                             \
	struct LuauStackOp<mType> {                             \
		static void Push(lua_State *L, const mType &value); \
                                                            \
		static mType Get(lua_State *L, int index);          \
		static bool Is(lua_State *L, int index);            \
		static mType Check(lua_State *L, int index);        \
	};                                                      \
                                                            \
	template <>                                             \
	struct LuauStackOp<mType *> {                           \
		static void Push(lua_State *L, const mType *value); \
                                                            \
		static mType *Get(lua_State *L, int index);         \
		static bool Is(lua_State *L, int index);            \
		static mType *Check(lua_State *L, int index);       \
	};

#define UDATA_STACK_OP_IMPL(mType, mMetatableName, mTag, mDtor)                                               \
	void LuauStackOp<mType *>::Push(lua_State *L, const mType *value) {                                       \
		LuauStackOp<mType>::Push(L, *value);                                                                  \
	}                                                                                                         \
                                                                                                              \
	void LuauStackOp<mType>::Push(lua_State *L, const mType &value) {                                         \
		/* FIXME: Shouldn't happen every time, but probably is fast */                                        \
		lua_setuserdatadtor(L, mTag, mDtor);                                                                  \
                                                                                                              \
		mType *udata = reinterpret_cast<mType *>(lua_newuserdatataggedwithmetatable(L, sizeof(mType), mTag)); \
		new (udata) mType();                                                                                  \
		*udata = value;                                                                                       \
	}                                                                                                         \
                                                                                                              \
	bool LuauStackOp<mType *>::Is(lua_State *L, int index) {                                                  \
		return lua_touserdatatagged(L, index, mTag);                                                          \
	}                                                                                                         \
                                                                                                              \
	bool LuauStackOp<mType>::Is(lua_State *L, int index) {                                                    \
		return LuauStackOp<mType *>::Is(L, index);                                                            \
	}                                                                                                         \
                                                                                                              \
	mType *LuauStackOp<mType *>::Get(lua_State *L, int index) {                                               \
		return reinterpret_cast<mType *>(lua_touserdatatagged(L, index, mTag));                               \
	}                                                                                                         \
                                                                                                              \
	mType LuauStackOp<mType>::Get(lua_State *L, int index) {                                                  \
		mType *udata = LuauStackOp<mType *>::Get(L, index);                                                   \
		if (!udata)                                                                                           \
			return mType();                                                                                   \
                                                                                                              \
		return *udata;                                                                                        \
	}                                                                                                         \
                                                                                                              \
	mType *LuauStackOp<mType *>::Check(lua_State *L, int index) {                                             \
		void *udata = lua_touserdatatagged(L, index, mTag);                                                   \
		if (!udata)                                                                                           \
			luaL_typeerrorL(L, index, mMetatableName);                                                        \
                                                                                                              \
		return reinterpret_cast<mType *>(udata);                                                              \
	}                                                                                                         \
                                                                                                              \
	mType LuauStackOp<mType>::Check(lua_State *L, int index) {                                                \
		return *LuauStackOp<mType *>::Check(L, index);                                                        \
	}

#define NO_DTOR [](lua_State *, void *) {}
#define DTOR(mType)                                 \
	[](lua_State *, void *udata) {                  \
		reinterpret_cast<mType *>(udata)->~mType(); \
	}

/* STATIC OBJECT: Use for objects not owned by Luau with ~static  lifetime */

#define STACK_OP_STATIC_PTR_DEF(type)                \
	template <>                                      \
	struct LuauStackOp<type *> {                     \
		static void Push(lua_State *L, type *value); \
                                                     \
		static type *Get(lua_State *L, int index);   \
		static bool Is(lua_State *L, int index);     \
		static type *Check(lua_State *L, int index); \
	};

#define STATIC_PTR_STACK_OP_IMPL(type, metatable_name, tag)                                                   \
	void LuauStackOp<type *>::Push(lua_State *L, type *value) {                                               \
		type **udata = reinterpret_cast<type **>(lua_newuserdatataggedwithmetatable(L, sizeof(void *), tag)); \
		*udata = value;                                                                                       \
	}                                                                                                         \
                                                                                                              \
	type *LuauStackOp<type *>::Get(lua_State *L, int index) {                                                 \
		type **udata = reinterpret_cast<type **>(lua_touserdatatagged(L, index, tag));                        \
		return udata ? *udata : nullptr;                                                                      \
	}                                                                                                         \
                                                                                                              \
	bool LuauStackOp<type *>::Is(lua_State *L, int index) {                                                   \
		return lua_touserdatatagged(L, index, tag);                                                           \
	}                                                                                                         \
                                                                                                              \
	type *LuauStackOp<type *>::Check(lua_State *L, int index) {                                               \
		type **udata = reinterpret_cast<type **>(lua_touserdatatagged(L, index, tag));                        \
		if (udata)                                                                                            \
			return *udata;                                                                                    \
		luaL_typeerrorL(L, index, metatable_name);                                                            \
	}

} //namespace SBX
