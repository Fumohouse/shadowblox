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

#include "Sbx/Classes/Variant.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Classes/Object.hpp"
#include "Sbx/DataTypes/EnumItem.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::Classes {

const Variant::Methods Variant::METHODS[TypeMax] = {
	// Null
	{
			nullptr,
			nullptr,
			nullptr,
			[](lua_State *L, const Variant & /* unused */) {
				lua_pushnil(L);
			},
			[](lua_State * /* unused */, int /* unused */) {
				return Variant();
			},
	},
	// Boolean
	{
			nullptr,
			&Variant::CopyFrom<bool>,
			&Variant::MoveFrom<bool>,
			[](lua_State *L, const Variant &value) {
				lua_pushboolean(L, *value.GetPtr<bool>());
			},
			[](lua_State *L, int index) -> Variant {
				return (bool)lua_toboolean(L, index);
			},
	},
	// Integer
	{
			nullptr,
			&Variant::CopyFrom<int64_t>,
			&Variant::MoveFrom<int64_t>,
			[](lua_State *L, const Variant &value) {
				LuauStackOp<int64_t>::Push(L, *value.GetPtr<int64_t>());
			},
			[](lua_State *L, int index) -> Variant {
				return LuauStackOp<int64_t>::Get(L, index);
			},
	},
	// Double
	{
			nullptr,
			&Variant::CopyFrom<double>,
			&Variant::MoveFrom<double>,
			[](lua_State *L, const Variant &value) {
				lua_pushnumber(L, *value.GetPtr<double>());
			},
			[](lua_State *L, int index) -> Variant {
				return lua_tonumber(L, index);
			},
	},
	// String
	{
			&Variant::Destroy<std::string>,
			&Variant::CopyFrom<std::string>,
			&Variant::MoveFrom<std::string>,
			[](lua_State *L, const Variant &value) {
				lua_pushstring(L, value.GetPtr<std::string>()->c_str());
			},
			[](lua_State *L, int index) -> Variant {
				return lua_tostring(L, index);
			},
	},

	// Function
	{
			&Variant::Destroy<LuauFunction>,
			&Variant::CopyFrom<LuauFunction>,
			&Variant::MoveFrom<LuauFunction>,
			[](lua_State *L, const Variant &value) {
				LuauStackOp<LuauFunction>::Push(L, *value.GetPtr<LuauFunction>());
			},
			[](lua_State *L, int index) -> Variant {
				return LuauStackOp<LuauFunction>::Get(L, index);
			},
	},
	// Dictionary
	{
			&Variant::Destroy<Classes::Dictionary>,
			&Variant::CopyFrom<Classes::Dictionary>,
			&Variant::MoveFrom<Classes::Dictionary>,
			[](lua_State *L, const Variant &value) {
				LuauStackOp<Classes::Dictionary>::Push(L, *value.GetPtr<Classes::Dictionary>());
			},
			[](lua_State *L, int index) -> Variant {
				return LuauStackOp<Classes::Dictionary>::Get(L, index);
			},
	},
	// Array
	{
			&Variant::Destroy<Classes::Array>,
			&Variant::CopyFrom<Classes::Array>,
			&Variant::MoveFrom<Classes::Array>,
			[](lua_State *L, const Variant &value) {
				LuauStackOp<Classes::Array>::Push(L, *value.GetPtr<Classes::Array>());
			},
			[](lua_State *L, int index) -> Variant {
				return LuauStackOp<Classes::Array>::Get(L, index);
			},
	},

	// EnumItem
	{
			nullptr,
			&Variant::CopyFrom<DataTypes::EnumItem *>,
			&Variant::MoveFrom<DataTypes::EnumItem *>,
			[](lua_State *L, const Variant &value) {
				LuauStackOp<DataTypes::EnumItem *>::Push(L, *value.GetPtr<DataTypes::EnumItem *>());
			},
			[](lua_State *L, int index) -> Variant {
				return LuauStackOp<DataTypes::EnumItem *>::Get(L, index);
			},
	},

	// Object
	{
			&Variant::Destroy<std::shared_ptr<Classes::Object>>,
			&Variant::CopyFrom<std::shared_ptr<Classes::Object>>,
			&Variant::MoveFrom<std::shared_ptr<Classes::Object>>,
			[](lua_State *L, const Variant &value) {
				LuauStackOp<std::shared_ptr<Classes::Object>>::Push(L, *value.GetPtr<std::shared_ptr<Classes::Object>>());
			},
			[](lua_State *L, int index) -> Variant {
				return LuauStackOp<std::shared_ptr<Classes::Object>>::Get(L, index);
			},
	},
};

Variant::Variant() {
}

void Variant::Clear() {
	if (auto method = METHODS[type].destroy) {
		(this->*method)();
	}

	type = Null;
}

Variant::~Variant() {
	Clear();
}

void Variant::Copy(const Variant &other) {
	if (other.type == Null) {
		type = Null;
		return;
	}

	type = other.type;
	(this->*METHODS[type].copyFrom)(other);
}

void Variant::Move(Variant &&other) {
	if (other.type == Null) {
		type = Null;
		return;
	}

	type = other.type;
	(this->*METHODS[type].moveFrom)(std::move(other));
	other.type = Null; // NOLINT
}

Variant::Variant(const Variant &other) {
	Copy(other);
}

Variant::Variant(Variant &&other) noexcept {
	Move(std::move(other));
}

Variant &Variant::operator=(const Variant &other) {
	if (&other == this) {
		return *this;
	}

	Clear();
	Copy(other);
	return *this;
}

Variant &Variant::operator=(Variant &&other) noexcept {
	if (&other == this) {
		return *this;
	}

	Clear();
	Move(std::move(other));
	return *this;
}

//////////////////
// LuauFunction //
//////////////////

LuauFunction::LuauFunction(lua_State *L, int index) {
	if (!lua_isfunction(L, index)) {
		return;
	}

	this->L = L;
	ref = lua_ref(L, index);
}

LuauFunction::operator bool() const {
	return L && ref != LUA_REFNIL;
}

void LuauFunction::Clear() {
	if (!*this) {
		return;
	}

	lua_unref(L, ref);
	L = nullptr;
	ref = LUA_REFNIL;
}

LuauFunction::~LuauFunction() {
	Clear();
}

void LuauFunction::Copy(const LuauFunction &other) {
	L = other.L;
	lua_getref(other.L, other.ref);
	ref = lua_ref(other.L, -1);
	lua_pop(L, 1);
}

void LuauFunction::Move(LuauFunction &&other) { // NOLINT
	L = other.L;
	ref = other.ref;
	other.L = nullptr;
	other.ref = LUA_REFNIL;
}

LuauFunction::LuauFunction(const LuauFunction &other) {
	Copy(other);
}

LuauFunction::LuauFunction(LuauFunction &&other) noexcept {
	Move(std::move(other));
}

LuauFunction &LuauFunction::operator=(const LuauFunction &other) {
	if (&other == this) {
		return *this;
	}

	Clear();
	Copy(other);
	return *this;
}

LuauFunction &LuauFunction::operator=(LuauFunction &&other) noexcept {
	if (&other == this) {
		return *this;
	}

	Clear();
	Move(std::move(other));
	return *this;
}

bool LuauFunction::Get(lua_State *T) const {
	if (!*this || lua_mainthread(T) != lua_mainthread(L)) {
		return false;
	}

	lua_getref(T, ref);
	return true;
}

} //namespace SBX::Classes

namespace SBX {

using Classes::Array;
using Classes::Dictionary;
using Classes::LuauFunction;
using Classes::Variant;

const std::string LuauStackOp<Variant>::NAME = "Variant";

void LuauStackOp<Variant>::Push(lua_State *L, const Variant &value) {
	Variant::METHODS[value.GetType()].push(L, value);
}

Variant::Type LuauStackOp<Variant>::GetType(lua_State *L, int index) {
	switch (lua_type(L, index)) {
		case LUA_TNIL:
			return Variant::Null;
		case LUA_TBOOLEAN:
			return Variant::Boolean;
		case LUA_TNUMBER:
			return Variant::Double;
		case LUA_TSTRING:
			return Variant::String;
		case LUA_TTABLE:
			if (LuauStackOp<Array>::Is(L, index)) {
				return Variant::Array;
			}

			return Variant::Dictionary;
		case LUA_TFUNCTION:
			return Variant::Function;
		case LUA_TUSERDATA: {
			if (LuauStackOp<int64_t>::Is(L, index)) {
				return Variant::Integer;
			}

			if (!lua_getmetatable(L, index)) {
				return Variant::TypeMax;
			}

			lua_getfield(L, -1, "__sbxtype");
			if (lua_isnil(L, -1)) {
				return Variant::TypeMax;
			}

			Variant::Type typeId = Variant::Type(lua_tointeger(L, -1));

			lua_pop(L, 2); // value, metatable
			return typeId;
		}

		default:
			break;
	}

	return Variant::TypeMax;
}

Variant LuauStackOp<Variant>::Get(lua_State *L, int index) {
	Variant::Type type = GetType(L, index);
	if (type == Variant::TypeMax) {
		return Variant();
	}

	return Variant::METHODS[type].get(L, index);
}

bool LuauStackOp<Variant>::Is(lua_State * /* unused */, int /* unused */) {
	// Accept all values, yielding Null on unsupported types
	return true;
}

Variant LuauStackOp<Variant>::Check(lua_State *L, int index) {
	return Get(L, index);
}

const std::string LuauStackOp<LuauFunction>::NAME = "Function";

void LuauStackOp<LuauFunction>::Push(lua_State *L, const LuauFunction &value) {
	value.Get(L);
}

LuauFunction LuauStackOp<LuauFunction>::Get(lua_State *L, int index) {
	return LuauFunction(L, index);
}

bool LuauStackOp<LuauFunction>::Is(lua_State *L, int index) {
	return lua_isfunction(L, index);
}

LuauFunction LuauStackOp<LuauFunction>::Check(lua_State *L, int index) {
	if (!Is(L, index)) {
		luaL_typeerrorL(L, index, NAME.c_str());
	}

	return LuauFunction(L, index);
}

const std::string LuauStackOp<Dictionary>::NAME = "Dictionary";

void LuauStackOp<Dictionary>::Push(lua_State *L, const Dictionary &value) {
	lua_createtable(L, 0, value.size());

	for (const auto &[k, v] : value) {
		LuauStackOp<Variant>::Push(L, v);
		lua_setfield(L, -2, k.c_str());
	}
}

Dictionary LuauStackOp<Dictionary>::Get(lua_State *L, int index) {
	if (!lua_istable(L, index)) {
		return Dictionary();
	}

	Dictionary res;

	index = lua_absindex(L, index);
	lua_pushnil(L);
	while (lua_next(L, index) != 0) {
		res[lua_tostring(L, -2)] = LuauStackOp<Variant>::Get(L, -1);
		lua_pop(L, 1); // value
	}

	return res;
}

bool LuauStackOp<Dictionary>::Is(lua_State *L, int index) {
	return lua_istable(L, index) && !LuauStackOp<Array>::Is(L, index);
}

Dictionary LuauStackOp<Dictionary>::Check(lua_State *L, int index) {
	if (!Is(L, index)) {
		luaL_typeerrorL(L, index, NAME.c_str());
	}

	return Get(L, index);
}

const std::string LuauStackOp<Array>::NAME = "Array";

void LuauStackOp<Array>::Push(lua_State *L, const Array &value) {
	lua_createtable(L, value.size(), 0);

	for (size_t i = 0; i < value.size(); i++) {
		LuauStackOp<Variant>::Push(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
}

Array LuauStackOp<Array>::Get(lua_State *L, int index) {
	if (!lua_istable(L, index)) {
		return Array();
	}

	int n = lua_objlen(L, index);
	Array res(n);

	for (int i = 1; i <= n; i++) {
		lua_rawgeti(L, index, i);
		res[i - 1] = LuauStackOp<Variant>::Get(L, -1);
		lua_pop(L, 1);
	}

	return res;
}

bool LuauStackOp<Array>::Is(lua_State *L, int index) {
	if (!lua_istable(L, index)) {
		return false;
	}

	if (lua_objlen(L, index) > 0) {
		return true;
	}

	index = lua_absindex(L, index);
	lua_pushnil(L);
	if (lua_next(L, index)) {
		// It's a Dictionary
		lua_pop(L, 2);
		return false;
	}

	return true;
}

Array LuauStackOp<Array>::Check(lua_State *L, int index) {
	if (!Is(L, index)) {
		luaL_typeerrorL(L, index, NAME.c_str());
	}

	return Get(L, index);
}

} //namespace SBX
