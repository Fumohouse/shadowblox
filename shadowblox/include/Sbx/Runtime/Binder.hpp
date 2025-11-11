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
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "lua.h"
#include "lualib.h"

#include "Sbx/DataTypes/EnumItem.hpp"
#include "Sbx/DataTypes/EnumTypes.gen.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX {

/**
 * @brief Throw this error inside functions bound to Luau to trigger a
 * luaL_error.
 * @see luaSBX_bindcxx
 */
class LuauBinderError : public std::exception {};

enum BindPurpose : uint8_t {
	BindFunction,
	BindGetter,
	BindSetter,
	BindOperator
};

// https://www.reddit.com/r/cpp_questions/comments/pumi9r/comment/he3swe8
template <auto N>
struct StringLiteral {
	char value[N]{};

	constexpr StringLiteral(const char (&str)[N]) {
		std::copy(str, str + N, value);
	}
};

template <typename T, StringLiteral name, BindPurpose purpose, int ofs>
inline T luaSBX_checkarg(lua_State *L, int index) {
	if constexpr (DataTypes::EnumClassToEnum<T>::enumType) {
		constexpr DataTypes::Enum *E = DataTypes::EnumClassToEnum<T>::enumType;

		if (lua_isnumber(L, index)) {
			int num = lua_tonumber(L, index);
			if (E->FromValue(num)) {
				return static_cast<T>(num);
			}

			if constexpr (purpose == BindSetter) {
				luaL_error(L, "Unable to assign property %s. Invalid value %d for enum %s", name.value, num, E->GetName());
			} else {
				luaSBX_casterror(L, luaL_typename(L, index), E->GetName());
			}

		} else if (lua_type(L, index) == LUA_TSTRING) {
			const char *value = lua_tostring(L, index);
			if (auto item = E->FromName(value)) {
				return static_cast<T>((*item)->GetValue());
			}

			if constexpr (purpose == BindSetter) {
				luaL_error(L, "Unable to assign property %s. Invalid value \"%s\" for enum %s", name.value, value, E->GetName());
			} else {
				luaSBX_casterror(L, luaL_typename(L, index), E->GetName());
			}

		} else if (LuauStackOp<DataTypes::EnumItem *>::Is(L, index)) {
			DataTypes::EnumItem *item = LuauStackOp<DataTypes::EnumItem *>::Get(L, index);
			if (item->GetEnumType() == E) {
				return static_cast<T>(item->GetValue());
			}

			if constexpr (purpose == BindSetter) {
				luaL_error(L, "Unable to assign property %s. EnumItem of type %s expected, got an EnumItem of type %s", name.value, E->GetName(), item->GetEnumType()->GetName());
			} else {
				luaSBX_casterror(L, item->GetEnumType()->GetName(), E->GetName());
			}

		} else {
			if constexpr (purpose == BindSetter) {
				luaL_error(L, "Unable to assign property %s. EnumItem, number, or string expected, got %s", name.value, luaL_typename(L, index));
			} else {
				if (lua_isnoneornil(L, index)) {
					luaSBX_missingargerror(L, index - ofs);
				} else {
					luaSBX_casterror(L, luaL_typename(L, index), E->GetName());
				}
			}
		}
	} else {
		if (LuauStackOp<T>::Is(L, index)) {
			return LuauStackOp<T>::Get(L, index);
		}

		if constexpr (purpose == BindSetter) {
			luaL_error(L, "Unable to assign property %s. %s expected, got %s", name.value, LuauStackOp<T>::NAME.c_str(), luaL_typename(L, index));
		} else {
			if (lua_isnoneornil(L, index)) {
				luaSBX_missingargerror(L, index - ofs);
			} else {
				luaSBX_casterror(L, luaL_typename(L, index), LuauStackOp<T>::NAME.c_str());
			}
		}
	}
}

// https://stackoverflow.com/a/48458312
template <typename>
struct IsTuple : std::false_type {};

template <typename... T>
struct IsTuple<std::tuple<T...>> : std::true_type {
	template <size_t... N>
	static int Push(lua_State *L, const std::tuple<T...> &val, std::index_sequence<N...> /*unused*/) {
		(LuauStackOp<T>::Push(L, std::get<N>(val)), ...);
		return sizeof...(T);
	}

	static int Push(lua_State *L, const std::tuple<T...> &val) {
		return Push(L, val, std::make_index_sequence<sizeof...(T)>());
	}
};

template <typename T>
inline int luaSBX_pushres(lua_State *L, T value) {
	if constexpr (IsTuple<T>::value) {
		return IsTuple<T>::Push(L, value);
	} else if constexpr (DataTypes::EnumClassToEnum<T>::enumType) {
		constexpr DataTypes::Enum *E = DataTypes::EnumClassToEnum<T>::enumType;
		int val = static_cast<int>(value);
		if (auto item = E->FromValue(val)) {
			LuauStackOp<DataTypes::EnumItem *>::Push(L, *item);
			return 1;
		}

		// Should be unreachable
		luaL_error(L, "enum value from C++ does not correspond to any Luau object");
	} else {
		LuauStackOp<T>::Push(L, value);
		return 1;
	}
}

// https://stackoverflow.com/a/70954691
template <typename Sig, StringLiteral name, BindPurpose purpose>
struct FuncType;

template <typename R, typename... Args, StringLiteral name, BindPurpose purpose>
struct FuncType<R (*)(Args...), name, purpose> {
	using FuncPtrType = R (*)(Args...);
	using ClassType = void;
	using RetType = R;
	using ArgTypes = std::tuple<Args...>;

	template <size_t... N>
	static R Invoke(lua_State *L, FuncPtrType func, std::index_sequence<N...> /*unused*/) {
		// C++ FUN FACT:
		// (): Parameter pack expansion may not be evaluated in order
		// {}: Evaluated in order
		// https://stackoverflow.com/a/42047998
		return std::apply(func, std::tuple{ luaSBX_checkarg<Args, name, purpose, 0>(L, N + 1)... });
	}

	static R Invoke(lua_State *L, FuncPtrType func) {
		return Invoke(L, func, std::make_index_sequence<sizeof...(Args)>());
	}
};

template <typename T, typename R, typename... Args, StringLiteral name, BindPurpose purpose>
struct FuncType<R (T::*)(Args...), name, purpose> {
	using FuncPtrType = R (T::*)(Args...);
	using ClassType = T;
	using RetType = R;
	using ArgTypes = std::tuple<Args...>;

	template <size_t... N>
	static R Invoke(lua_State *L, FuncPtrType func, std::index_sequence<N...> /*unused*/) {
		T *self = LuauStackOp<T *>::Get(L, 1);
		if constexpr (purpose == BindFunction) {
			if (!self) {
				luaSBX_missingselferror(L, name.value);
			}
		}
		std::tuple args{ luaSBX_checkarg<Args, name, purpose, 1>(L, N + 2)... };
		return std::invoke(func, self, std::get<N>(args)...);
	}

	static R Invoke(lua_State *L, FuncPtrType func) {
		return Invoke(L, func, std::make_index_sequence<sizeof...(Args)>());
	}
};

template <typename T, typename R, typename... Args, StringLiteral name, BindPurpose purpose>
struct FuncType<R (T::*)(Args...) const, name, purpose> {
	using FuncPtrType = R (T::*)(Args...) const;
	using ClassType = T;
	using RetType = R;
	using ArgTypes = std::tuple<Args...>;

	template <size_t... N>
	static R Invoke(lua_State *L, FuncPtrType func, std::index_sequence<N...> /*unused*/) {
		T *self = LuauStackOp<T *>::Get(L, 1);
		if constexpr (purpose == BindFunction) {
			if (!self) {
				luaSBX_missingselferror(L, name.value);
			}
		}
		std::tuple args{ luaSBX_checkarg<Args, name, purpose, 1>(L, N + 2)... };
		return std::invoke(func, self, std::get<N>(args)...);
	}

	static R Invoke(lua_State *L, FuncPtrType func) {
		return Invoke(L, func, std::make_index_sequence<sizeof...(Args)>());
	}
};

/**
 * @brief A template function that converts a C++ function to a function
 * directly bindable to Luau (`lua_CFunction`).
 *
 * The function may be a non-member, static member, const member, or non-const
 * member function. If the function returns a `std::tuple`, it will be pushed
 * to Luau as separate return values.
 *
 * @tparam name       The name of the function/property to use for capability
 *                    checks.
 * @tparam F          The function.
 * @tparam capability Capabilities required to run this function.
 * @tparam TF         The type of the function. Automatically populated.
 * @tparam purpose    The purpose of this bound function (e.g., function or
 *                    property getter/setter). The capability error message is
 *                    adjusted accordingly.
 */
template <StringLiteral name, auto F, SbxCapability capability, BindPurpose purpose = BindFunction, typename TF = decltype(F)>
int luaSBX_bindcxx(lua_State *L) {
	if constexpr (capability != NoneSecurity) {
		static const char *purposes[] = {
			"call",
			"read",
			"write",
			"use operator"
		};

		if constexpr (name.value[0]) {
			static std::string action = std::string(purposes[purpose]) + " '" + name.value + '\'';
			luaSBX_checkcapability(L, capability, action.c_str());
		} else {
			luaSBX_checkcapability(L, capability, purposes[purpose]);
		}
	}

	using FT = FuncType<TF, name, purpose>;

	try {
		if constexpr (std::is_same<typename FT::RetType, void>()) {
			FT::Invoke(L, F);
			return 0;
		} else {
			return luaSBX_pushres<typename FT::RetType>(L, FT::Invoke(L, F));
		}
	} catch (LuauBinderError &e) {
		luaL_error(L, "%s", e.what());
	}
}

} //namespace SBX
