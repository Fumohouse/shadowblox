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

#include <algorithm>
#include <exception>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX {

/**
 * Throw this error inside classes bound to Luau to trigger a luaL_error.
 *
 * @see luaSBX_bindcxx
 */
//
class LuauClassError : public std::exception {};

template <typename T>
T luaSBX_checkarg(lua_State *L, int &index) {
	return LuauStackOp<T>::Check(L, index++);
}

template <>
lua_State *luaSBX_checkarg<lua_State *>(lua_State *L, int &index) {
	return L;
}

// https://stackoverflow.com/a/70954691
template <typename Sig>
struct FuncType;

template <typename R, typename... Args>
struct FuncType<R (*)(Args...)> {
	using ClassType = void;
	using RetType = R;
	using ArgTypes = std::tuple<Args...>;

	static inline std::tuple<Args...> CheckArgs(lua_State *L, int start = 1) {
		// C++ FUN FACT:
		// (): Parameter pack expansion may not be evaluated in order
		// {}: Evaluated in order
		// https://stackoverflow.com/a/42047998
		return std::tuple{ luaSBX_checkarg<Args>(L, start)... };
	}
};

template <typename T, typename R, typename... Args>
struct FuncType<R (T::*)(Args...)> {
	using ClassType = T;
	using RetType = R;
	using ArgTypes = std::tuple<Args...>;

	static inline T *CheckThis(lua_State *L) {
		return LuauStackOp<T *>::Check(L, 1);
	}

	static inline std::tuple<Args...> CheckArgs(lua_State *L, int start = 2) {
		return std::tuple{ luaSBX_checkarg<Args>(L, start)... };
	}
};

template <typename T, typename R, typename... Args>
struct FuncType<R (T::*)(Args...) const> {
	using ClassType = T;
	using RetType = R;
	using ArgTypes = std::tuple<Args...>;

	static inline const T *CheckThis(lua_State *L) {
		return LuauStackOp<T *>::Check(L, 1);
	}

	static inline std::tuple<Args...> CheckArgs(lua_State *L, int start = 2) {
		return std::tuple{ luaSBX_checkarg<Args>(L, start)... };
	}
};

// https://www.reddit.com/r/cpp_questions/comments/pumi9r/comment/he3swe8
template <auto N>
struct StringLiteral {
	char value[N];

	constexpr StringLiteral(const char (&str)[N]) {
		std::copy(str, str + N, value);
	}
};

/**
 * A template function that converts a C++ function to a function directly
 * bindable to Luau (lua_CFunction).
 *
 * @tparam name       The name of the function to use for capability checks.
 * @tparam F          The function. Can be a non-member, static member, const
 *                    member, or non-const member function.
 * @tparam capability Capabilities required to run this function.
 * @tparam TF         The type of the function. Automatically populated.
 */
template <StringLiteral name, auto F, SbxCapability capability, typename TF = decltype(F)>
int luaSBX_bindcxx(lua_State *L) {
	static std::string action = std::string("call '") + name.value + '\'';
	luaSBX_checkcapability(L, capability, action.c_str());

	try {
		if constexpr (std::is_same<typename FuncType<TF>::ClassType, void>()) {
			// Static and non-member functions
			if constexpr (std::is_same<typename FuncType<TF>::RetType, void>()) {
				std::apply(F, FuncType<TF>::CheckArgs(L));
				return 0;
			} else {
				LuauStackOp<typename FuncType<TF>::RetType>::Push(L, std::apply(F, FuncType<TF>::CheckArgs(L)));
				return 1;
			}
		} else {
			// Member functions
			// https://stackoverflow.com/a/69694086
			auto *self = FuncType<TF>::CheckThis(L);
			if constexpr (std::is_same<typename FuncType<TF>::RetType, void>()) {
				std::apply(std::bind_front(F, self), FuncType<TF>::CheckArgs(L));
				return 0;
			} else {
				LuauStackOp<typename FuncType<TF>::RetType>::Push(L, std::apply(std::bind_front(F, self), FuncType<TF>::CheckArgs(L)));
				return 1;
			}
		}
	} catch (LuauClassError &e) {
		luaL_error(L, "%s", e.what());
	}
}

} //namespace SBX
