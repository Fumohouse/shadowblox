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
#include <vector>

#include "lua.h"
#include "lualib.h"

namespace SBX {

template <typename T>
struct LuauStackOp {};

template <typename T>
struct LuauStackOp<const T *> {
	static const std::string NAME;

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
const std::string LuauStackOp<const T *>::NAME = LuauStackOp<T>::NAME;

template <typename T>
struct LuauStackOp<T &> {
	static const std::string NAME;

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
const std::string LuauStackOp<T &>::NAME = LuauStackOp<T>::NAME;

template <typename T>
struct LuauStackOp<const T &> {
	static const std::string NAME;

	static void Push(lua_State *L, const T &value) {
		LuauStackOp<T>::Push(L, value);
	}

	static const T &Get(lua_State *L, int index) {
		return *LuauStackOp<T *>::Get(L, index);
	}

	static bool Is(lua_State *L, int index) {
		return LuauStackOp<T>::Is(L, index);
	}

	static const T &Check(lua_State *L, int index) {
		return *LuauStackOp<T *>::Check(L, index);
	}
};

template <typename T>
const std::string LuauStackOp<const T &>::NAME = LuauStackOp<T>::NAME;

template <typename T>
struct LuauStackOp<std::optional<T>> {
	static const std::string NAME;

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
		}

		return std::nullopt;
	}

	static bool Is(lua_State *L, int index) {
		return true;
	}

	static std::optional<T> Check(lua_State *L, int index) {
		return Get(L, index);
	}
};

template <typename T>
const std::string LuauStackOp<std::optional<T>>::NAME = LuauStackOp<T>::NAME + "?";

template <typename T>
struct LuauStackOp<std::vector<T>> {
	static const std::string NAME;

	static void Push(lua_State *L, const std::vector<T> &value) {
		lua_newtable(L);

		for (int i = 0; i < value.size(); i++) {
			LuauStackOp<T>::Push(L, value[i]);
			lua_rawseti(L, -2, i + 1);
		}
	}

	static std::vector<T> Get(lua_State *L, int index) {
		std::vector<T> res;
		if (!lua_istable(L, index)) {
			return res;
		}

		int n = lua_objlen(L, index);
		for (int i = 1; i <= n; i++) {
			lua_rawgeti(L, index, i);
			if (!LuauStackOp<T>::Is(L, -1)) {
				lua_pop(L, 1);
				return std::vector<T>();
			}

			res.push_back(LuauStackOp<T>::Get(L, -1));
			lua_pop(L, 1);
		}

		return res;
	}

	static bool Is(lua_State *L, int index) {
		if (!lua_istable(L, index)) {
			return false;
		}

		index = lua_absindex(L, index);

		int n = lua_objlen(L, index);
		for (int i = 1; i <= n; i++) {
			lua_rawgeti(L, index, i);
			if (!LuauStackOp<T>::Is(L, -1)) {
				lua_pop(L, 1);
				return false;
			}

			lua_pop(L, 1);
		}

		return true;
	}

	static std::vector<T> Check(lua_State *L, int index) {
		if (!lua_istable(L, index)) {
			luaL_typeerrorL(L, index, NAME.c_str());
		}

		std::vector<T> res;

		int n = lua_objlen(L, index);
		for (int i = 1; i <= n; i++) {
			lua_rawgeti(L, index, i);
			if (!LuauStackOp<T>::Is(L, -1)) {
				luaL_typeerrorL(L, index, NAME.c_str());
			}

			res.push_back(LuauStackOp<T>::Get(L, -1));
			lua_pop(L, 1);
		}

		return res;
	}
};

template <typename T>
const std::string LuauStackOp<std::vector<T>>::NAME = "Array";

#define STACK_OP_DEF(mType)                                 \
	template <>                                             \
	struct LuauStackOp<mType> {                             \
		static const std::string NAME;                      \
                                                            \
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
	static const std::string NAME;

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
	static const std::string NAME;

	static void Push(lua_State *L, const char *value);

	static const char *Get(lua_State *L, int index);
	static bool Is(lua_State *L, int index);
	static const char *Check(lua_State *L, int index);
};

/* USERDATA: Use for (typ. immutable) objects owned by Luau */

#define STACK_OP_UDATA_DEF(type)                           \
	template <>                                            \
	struct LuauStackOp<type> {                             \
		static const std::string NAME;                     \
                                                           \
		static void Push(lua_State *L, const type &value); \
                                                           \
		static type Get(lua_State *L, int index);          \
		static bool Is(lua_State *L, int index);           \
		static type Check(lua_State *L, int index);        \
	};                                                     \
                                                           \
	template <>                                            \
	struct LuauStackOp<type *> {                           \
		static const std::string NAME;                     \
                                                           \
		static void Push(lua_State *L, const type *value); \
                                                           \
		static type *Get(lua_State *L, int index);         \
		static bool Is(lua_State *L, int index);           \
		static type *Check(lua_State *L, int index);       \
	};

#define UDATA_STACK_OP_IMPL(type, name, metatableName, tag, dtor)                                         \
	void LuauStackOp<type *>::Push(lua_State *L, const type *value) {                                     \
		LuauStackOp<type>::Push(L, *value);                                                               \
	}                                                                                                     \
                                                                                                          \
	void LuauStackOp<type>::Push(lua_State *L, const type &value) {                                       \
		/* FIXME: Shouldn't happen every time, but probably is fast */                                    \
		lua_setuserdatadtor(L, tag, dtor);                                                                \
                                                                                                          \
		type *udata = reinterpret_cast<type *>(lua_newuserdatataggedwithmetatable(L, sizeof(type), tag)); \
		new (udata) type();                                                                               \
		*udata = value;                                                                                   \
	}                                                                                                     \
                                                                                                          \
	bool LuauStackOp<type *>::Is(lua_State *L, int index) {                                               \
		return lua_touserdatatagged(L, index, tag);                                                       \
	}                                                                                                     \
                                                                                                          \
	bool LuauStackOp<type>::Is(lua_State *L, int index) {                                                 \
		return LuauStackOp<type *>::Is(L, index);                                                         \
	}                                                                                                     \
                                                                                                          \
	type *LuauStackOp<type *>::Get(lua_State *L, int index) {                                             \
		return reinterpret_cast<type *>(lua_touserdatatagged(L, index, tag));                             \
	}                                                                                                     \
                                                                                                          \
	const std::string LuauStackOp<type *>::NAME = name;                                                   \
                                                                                                          \
	type LuauStackOp<type>::Get(lua_State *L, int index) {                                                \
		type *udata = LuauStackOp<type *>::Get(L, index);                                                 \
		if (!udata)                                                                                       \
			return type();                                                                                \
                                                                                                          \
		return *udata;                                                                                    \
	}                                                                                                     \
                                                                                                          \
	type *LuauStackOp<type *>::Check(lua_State *L, int index) {                                           \
		void *udata = lua_touserdatatagged(L, index, tag);                                                \
		if (!udata)                                                                                       \
			luaL_typeerrorL(L, index, metatableName);                                                     \
                                                                                                          \
		return reinterpret_cast<type *>(udata);                                                           \
	}                                                                                                     \
                                                                                                          \
	type LuauStackOp<type>::Check(lua_State *L, int index) {                                              \
		return *LuauStackOp<type *>::Check(L, index);                                                     \
	}                                                                                                     \
                                                                                                          \
	const std::string LuauStackOp<type>::NAME = name;

#define NO_DTOR [](lua_State *, void *) {}
#define DTOR(type)                                \
	[](lua_State *, void *udata) {                \
		reinterpret_cast<type *>(udata)->~type(); \
	}

/* STATIC OBJECT: Use for objects not owned by Luau with ~static lifetime */

#define STACK_OP_STATIC_PTR_DEF(type)                \
	template <>                                      \
	struct LuauStackOp<type *> {                     \
		static const std::string NAME;               \
                                                     \
		static void Push(lua_State *L, type *value); \
                                                     \
		static type *Get(lua_State *L, int index);   \
		static bool Is(lua_State *L, int index);     \
		static type *Check(lua_State *L, int index); \
	};

#define STATIC_PTR_STACK_OP_IMPL(type, name, metatable_name, tag)                                             \
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
	}                                                                                                         \
                                                                                                              \
	const std::string LuauStackOp<type *>::NAME = name

/* STATIC REGREF: Use for ~static objects with strong/weak cache in Luau */

#define STACK_OP_REGISTRY_PTR_DEF(type)                                 \
	template <>                                                         \
	struct LuauStackOp<type *> {                                        \
		static const std::string NAME;                                  \
                                                                        \
		static void PushRaw(lua_State *L, void *value, void *userdata); \
		static void Push(lua_State *L, type *value);                    \
                                                                        \
		static type *Get(lua_State *L, int index);                      \
		static bool Is(lua_State *L, int index);                        \
		static type *Check(lua_State *L, int index);                    \
	};

#define REGISTRY_PTR_STACK_OP_IMPL(type, name, metatable_name, tag, weak)                                     \
	void LuauStackOp<type *>::PushRaw(lua_State *L, void *value, void * /* unused */) {                       \
		type **udata = reinterpret_cast<type **>(lua_newuserdatataggedwithmetatable(L, sizeof(void *), tag)); \
		*udata = (type *)value;                                                                               \
	}                                                                                                         \
                                                                                                              \
	void LuauStackOp<type *>::Push(lua_State *L, type *value) {                                               \
		luaSBX_pushregistry(L, value, nullptr, PushRaw, weak);                                                \
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
	}                                                                                                         \
                                                                                                              \
	const std::string LuauStackOp<type *>::NAME = name

} //namespace SBX
