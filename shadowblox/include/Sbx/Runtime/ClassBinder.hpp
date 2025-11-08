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

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "ltm.h"
#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Binder.hpp"

namespace SBX {

/**
 * @brief Utility class to bind C++ classes to Luau.
 *
 * @tparam T The class being bound to Luau. All binding data is stored based on
 *           this type.
 */
template <typename T>
class LuauClassBinder {
public:
	using TypePredicate = bool (*)(lua_State *L, int index);
	using IndexOverride = int (*)(lua_State *L, const char *name);

	/**
	 * @brief Initialize the class binder.
	 *
	 * @param newName          The name of this class. Must be non-empty.
	 * @param newMetatableName The metatable name of this class.
	 * @param newUdataTag      The userdata tag for this class. If negative, the
	 *                         metatable is registered by name only.
	 */
	static void Init(const char *newName, const char *newMetatableName, int newUdataTag = -1) {
		name = newName;
		metatableName = newMetatableName;
		udataTag = newUdataTag;
	}

	/**
	 * @brief Returns whether the class has been previously initialized through
	 * `Init`.
	 *
	 * @returns Initialization status.
	 * @see Init
	 */
	static bool IsInitialized() {
		return *name;
	}

	/**
	 * @brief Register the global table for this class.
	 *
	 * The global table contains all static methods of this class.
	 *
	 * @param L The Luau state.
	 */
	static void InitGlobalTable(lua_State *L) {
		lua_newtable(L);

		for (const auto &[name, method] : staticMethods) {
			lua_pushcfunction(L, method.func, method.debugName.c_str());
			lua_setfield(L, -2, name.c_str());
		}

		lua_setreadonly(L, -1, true);
		lua_setglobal(L, name);
	}

	/**
	 * @brief Register the metatable for this class.
	 *
	 * Methods, properties, and unary/binary/call operators are registered
	 * automatically. If provided, the userdata tag is set at this step.
	 * Otherwise, the metatable is stored in a global table by name only.
	 *
	 * @param L The Luau state.
	 */
	static void InitMetatable(lua_State *L) {
		if (!luaL_newmetatable(L, metatableName)) {
			luaL_error(L, "metatable '%s' already exists", metatableName);
		}

		lua_pushstring(L, name);
		lua_setfield(L, -2, "__type");

		lua_pushstring(L, "The metatable is locked");
		lua_setfield(L, -2, "__metatable");

		lua_pushcfunction(L, Namecall, nullptr);
		lua_setfield(L, -2, "__namecall");

		lua_pushcfunction(L, Newindex, nullptr);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, Index, nullptr);
		lua_setfield(L, -2, "__index");

		if (tostringFunc) {
			lua_pushcfunction(L, tostringFunc, nullptr);
			lua_setfield(L, -2, "__tostring");
		}

		if (callOperator) {
			lua_pushcfunction(L, callOperator, nullptr);
			lua_setfield(L, -2, "__call");
		}

		for (const auto &[tms, func] : operatorFunctions) {
			const char *opName = luaT_eventname[tms];
			lua_pushcfunction(L, func, nullptr);
			lua_setfield(L, -2, opName);
		}

		lua_setreadonly(L, -1, true);
		if (udataTag >= 0)
			lua_setuserdatametatable(L, udataTag);
		else
			lua_pop(L, 1);
	}

	/**
	 * @brief Bind a non-member function to Luau, to be registered on the class
	 * global table.
	 *
	 * The function should be a non-member that takes one or more arguments and
	 * optionally returns a value.
	 *
	 * @tparam funcName   The function's name.
	 * @tparam F          The function.
	 * @tparam capability The capabilities required to use the function.
	 *
	 * @see InitGlobalTable
	 */
	template <StringLiteral funcName, auto F, SbxCapability capability>
	static void BindStaticMethod() {
		staticMethods[funcName.value] = {
			luaSBX_bindcxx<funcName, F, capability>,
			std::string(name) + funcName.value
		};
	}

	/**
	 * @brief Bind a Luau C function as a non-member function, to be registered
	 * on the global table.
	 *
	 * @tparam funcName   The function's name.
	 * @tparam F          The function.
	 *
	 * @see InitGlobalTable
	 */
	template <StringLiteral funcName, lua_CFunction F>
	static void BindLuauStaticMethod() {
		staticMethods[funcName.value] = {
			F,
			std::string(name) + funcName.value
		};
	}

	/**
	 * @brief Bind a member function to Luau.
	 *
	 * The function should be a member or non-member that takes one or more
	 * arguments, `this` and any further arguments for the function, and
	 * optionally returns a value.
	 *
	 * @tparam funcName   The function's name.
	 * @tparam F          The function.
	 * @tparam capability The capabilities required to use the function.
	 */
	template <StringLiteral funcName, auto F, SbxCapability capability>
	static void BindMethod() {
		methods[funcName.value] = {
			luaSBX_bindcxx<funcName, F, capability>,
			std::string(name) + funcName.value
		};
	}

	/**
	 * @brief Bind a Luau C function as a member function.
	 *
	 * @tparam funcName   The function's name.
	 * @tparam F          The function.
	 */
	template <StringLiteral funcName, lua_CFunction F>
	static void BindLuauMethod() {
		methods[funcName.value] = {
			F,
			std::string(name) + funcName.value
		};
	}

	/**
	 * @brief Bind a read-write property to Luau.
	 *
	 * The getter function should be a member or non-member function that takes
	 * only one argument, `this`, and returns the property value. The setter
	 * function should be a member or non-member function that takes two
	 * arguments, `this` and the new value, and returns nothing.
	 *
	 * @tparam propName      The property name.
	 * @tparam G             The getter function.
	 * @tparam getCapability Capabilities required to read the property.
	 * @tparam S             The setter function.
	 * @tparam setCapability Capabilities required to write to the property.
	 */
	template <StringLiteral propName, auto G, SbxCapability getCapability, auto S, SbxCapability setCapability>
	static void BindProperty() {
		properties[propName.value] = {
			luaSBX_bindcxx<propName, G, getCapability, BindGetter>,
			luaSBX_bindcxx<propName, S, setCapability, BindSetter>
		};
	}

	/**
	 * @brief Bind a read-only property to Luau.
	 *
	 * The getter function should be a member or non-member function that takes
	 * only one argument, `this`, and returns the property value.
	 *
	 * @tparam propName      The property name.
	 * @tparam G             The getter function.
	 * @tparam getCapability Capabilities required to read the property.
	 */
	template <StringLiteral propName, auto G, SbxCapability getCapability>
	static void BindPropertyReadOnly() {
		properties[propName.value] = {
			luaSBX_bindcxx<propName, G, getCapability, BindGetter>,
			nullptr
		};
	}

	/**
	 * @brief Bind a write-only property to Luau.
	 *
	 * The setter function should be a member or non-member function that takes
	 * two arguments, `this` and the new value, and returns nothing.
	 *
	 * @tparam propName      The property name.
	 * @tparam S             The setter function.
	 * @tparam setCapability Capabilities required to write to the property.
	 */
	template <StringLiteral propName, auto S, SbxCapability setCapability>
	static void BindPropertyWriteOnly() {
		properties[propName.value] = {
			nullptr,
			luaSBX_bindcxx<propName, S, setCapability, BindSetter>
		};
	}

	/**
	 * @brief Bind a `__tostring` operator to Luau.
	 *
	 * The function should be a member or non-member function that takes only
	 * one argument, `this`, and returns a string.
	 *
	 * @tparam F          The function.
	 * @tparam capability Required capabilities to use this operator.
	 */
	template <auto F, SbxCapability capability = NoneSecurity>
	static void BindToString() {
		tostringFunc = luaSBX_bindcxx<"", F, capability, BindOperator>;
	}

	/**
	 * @brief Bind a call operator to Luau.
	 *
	 * The supplied function should be a member or non-member function that
	 * takes in any number of arguments, including `this` and any further
	 * arguments for the call operation, and optionally returns a value.
	 *
	 * @tparam F          The function.
	 * @tparam capability Required capabilities to use this operator.
	 */
	template <auto F, SbxCapability capability = NoneSecurity>
	static void BindCallOp() {
		callOperator = luaSBX_bindcxx<"", F, capability, BindOperator>;
	}

	/**
	 * @brief Set an `__index` override for this class.
	 *
	 * The supplied function should take two parameters, the Luau state and the
	 * requested property, and return the number of resulting items pushed onto
	 * the stack. If the number of items is zero, the function results will
	 * be ignored.
	 *
	 * @param func The function.
	 */
	static void SetIndexOverride(IndexOverride func) {
		indexOverride = func;
	}

	/**
	 * @brief Bind a binary operator to Luau.
	 *
	 * The supplied function should be a member or non-member function that
	 * takes only two arguments, the LHS and RHS, and returns a value. For
	 * arithmetic operators, a non-member `operator` function is recommended.
	 *
	 * @tparam type       The type of operator, e.g., `TM_ADD`.
	 * @tparam F          The function.
	 * @tparam capability Required capabilities to use this operator.
	 * @param lhsPredicate A function that returns `true` when the LHS is the
	 *                     correct type.
	 * @param rhsPredicate A function that returns `true` when the RHS is the
	 *                     correct type.
	 */
	template <TMS type, auto F, SbxCapability capability = NoneSecurity>
	static void BindBinaryOp(TypePredicate lhsPredicate, TypePredicate rhsPredicate) {
		if (binaryOperators.find(type) == binaryOperators.end()) {
			operatorFunctions[type] = BinaryOperator<type>;
			binaryOperators[type] = std::vector<BinOp>();
		}

		binaryOperators[type].push_back({
				luaSBX_bindcxx<"", F, capability, BindOperator>,
				lhsPredicate,
				rhsPredicate,
		});
	}

	/**
	 * @brief Bind a unary operator to Luau.
	 *
	 * The supplied function should be a member or non-member function that
	 * takes only one argument, `this`, and returns a value.
	 *
	 * @tparam type       The type of operator, e.g., `TM_UNM`.
	 * @tparam F          The function.
	 * @tparam capability Required capabilities to use this operator.
	 */
	template <TMS type, auto F, SbxCapability capability = NoneSecurity>
	static void BindUnaryOp() {
		operatorFunctions[type] = luaSBX_bindcxx<"", F, capability, BindOperator>;
	}

private:
	static const char *name;
	static const char *metatableName;
	static int udataTag;

	struct Method {
		lua_CFunction func;
		std::string debugName;
	};

	struct Property {
		lua_CFunction getter;
		lua_CFunction setter;
	};

	struct BinOp {
		lua_CFunction func;
		TypePredicate lhsPredicate;
		TypePredicate rhsPredicate;
	};

	static std::unordered_map<std::string, Method> staticMethods;
	static std::unordered_map<std::string, Method> methods;
	static std::unordered_map<std::string, Property> properties;

	static lua_CFunction tostringFunc;
	static lua_CFunction callOperator;
	static IndexOverride indexOverride;

	// For binary operators: Wrapper functions that call into binaryOperators
	// (only one per operator)
	// For unary operators: The functions themselves
	static std::unordered_map<TMS, lua_CFunction> operatorFunctions;
	static std::unordered_map<TMS, std::vector<BinOp>> binaryOperators;

	/* Metamethods */
	static int Namecall(lua_State *L) {
		if (const char *methodName = lua_namecallatom(L, nullptr)) {
			auto it = methods.find(methodName);
			if (it != methods.end()) {
				return it->second.func(L);
			}

			luaSBX_nomethoderror(L, methodName, name);
		}

		luaSBX_nonamecallatomerror(L);
	}

	static int Index(lua_State *L) {
		const char *propName = luaL_checkstring(L, 2);

		if (indexOverride) {
			if (int nret = indexOverride(L, propName)) {
				return nret;
			}
		}

		auto it = properties.find(propName);
		if (it != properties.end()) {
			if (!it->second.getter) {
				luaSBX_propwriteonlyerror(L, propName, name);
			}

			lua_remove(L, 2);
			return it->second.getter(L);
		}

		auto it2 = methods.find(propName);
		if (it2 != methods.end()) {
			lua_pushcfunction(L, it2->second.func, it2->second.debugName.c_str());
			return 1;
		}

		luaSBX_noproperror(L, propName, name);
	}

	static int Newindex(lua_State *L) {
		const char *propName = luaL_checkstring(L, 2);

		auto it = properties.find(propName);
		if (it != properties.end()) {
			if (!it->second.setter) {
				luaSBX_propreadonlyerror(L, propName, name);
			}

			lua_remove(L, 2);
			return it->second.setter(L);
		}

		luaSBX_noproperror(L, propName, name);
	}

	template <TMS type>
	static int BinaryOperator(lua_State *L) {
		const char *OP = luaT_eventname[type];
		const std::vector<BinOp> &overloads = binaryOperators[type];

		for (const BinOp &op : overloads) {
			if (op.lhsPredicate(L, 1) && op.rhsPredicate(L, 2)) {
				return op.func(L);
			}
		}

		const char *lhsName = luaL_typename(L, 1);
		const char *rhsName = luaL_typename(L, 2);

		if (strcmp(lhsName, rhsName) == 0) {
			luaSBX_aritherror1type(L, OP + 2, lhsName);
		} else {
			luaSBX_aritherror2type(L, OP + 2, lhsName, rhsName);
		}
	}
};

template <typename T>
const char *LuauClassBinder<T>::name = "";

template <typename T>
const char *LuauClassBinder<T>::metatableName = "";

template <typename T>
int LuauClassBinder<T>::udataTag = -1;

template <typename T>
std::unordered_map<std::string, typename LuauClassBinder<T>::Method> LuauClassBinder<T>::staticMethods;

template <typename T>
std::unordered_map<std::string, typename LuauClassBinder<T>::Method> LuauClassBinder<T>::methods;

template <typename T>
std::unordered_map<std::string, typename LuauClassBinder<T>::Property> LuauClassBinder<T>::properties;

template <typename T>
lua_CFunction LuauClassBinder<T>::tostringFunc = nullptr;

template <typename T>
lua_CFunction LuauClassBinder<T>::callOperator = nullptr;

template <typename T>
LuauClassBinder<T>::IndexOverride LuauClassBinder<T>::indexOverride = nullptr;

template <typename T>
std::unordered_map<TMS, lua_CFunction> LuauClassBinder<T>::operatorFunctions;

template <typename T>
std::unordered_map<TMS, std::vector<typename LuauClassBinder<T>::BinOp>> LuauClassBinder<T>::binaryOperators;

} //namespace SBX
