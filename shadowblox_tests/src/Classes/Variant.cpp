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

#include "doctest.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "lua.h"

#include "Sbx/Classes/ClassDB.hpp"
#include "Sbx/Classes/Object.hpp"
#include "Sbx/Classes/Variant.hpp"
#include "Sbx/DataTypes/EnumItem.hpp"
#include "Sbx/DataTypes/Types.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Stack.hpp"
#include "Utils.hpp"

using namespace SBX;
using namespace SBX::Classes;

TEST_SUITE_BEGIN("Classes/Variant");

namespace SBX::Classes {

class TestVariant : public Object {
	SBXCLASS(TestVariant, Object, MemoryCategory::Internal);

public:
	TestVariant() {
	}

	~TestVariant() override {
	}

protected:
	template <typename T>
	static void BindMembers() {
		Object::BindMembers<T>();
	}
};

} //namespace SBX::Classes

TEST_CASE("LuauFunction") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);

	LuauFunction none;
	CHECK_FALSE(none);

	EVAL_THEN(L, "return function() end", {
		REQUIRE(lua_isfunction(L, -1));

		// Track GC lifetime of the function
		lua_newtable(L);
		lua_newtable(L);
		lua_pushstring(L, "v");
		lua_setfield(L, -2, "__mode");
		lua_setmetatable(L, -2);

		lua_pushvalue(L, -2);
		lua_setfield(L, -2, "func");

		lua_getfield(L, -1, "func");
		REQUIRE(lua_isfunction(L, -1));
		lua_pop(L, 1);

		LuauFunction func(L, -2);
		REQUIRE(func);

		lua_remove(L, -2);
		lua_gc(L, LUA_GCCOLLECT, 0);

		LuauFunction funcCopy(func);
		REQUIRE(funcCopy);

		func.Get(L);
		CHECK(lua_isfunction(L, -1));
		lua_pop(L, 1);

		funcCopy.Get(L);
		CHECK(lua_isfunction(L, -1));
		lua_pop(L, 1);

		LuauFunction funcMoved = std::move(func);
		CHECK_FALSE(func); // NOLINT
		REQUIRE(funcMoved);

		funcMoved.Get(L);
		CHECK(lua_isfunction(L, -1));
		lua_pop(L, 1);

		func.Clear();
		funcCopy.Clear();
		funcMoved.Clear();
		lua_gc(L, LUA_GCCOLLECT, 0);

		lua_getfield(L, -1, "func");
		CHECK(lua_isnil(L, -1));
	});

	luaSBX_close(L);
}

template <typename T>
static void testVariant(const T &val, Variant::Type type) {
	// Construction
	Variant v1(val);
	REQUIRE_EQ(v1.GetType(), type);
	CHECK_EQ(*v1.GetPtr<T>(), val);

	// Value assignment
	Variant v2;
	v2 = val;
	REQUIRE_EQ(v2.GetType(), type);
	CHECK_EQ(*v2.GetPtr<T>(), val);

	// Copy constructor
	Variant v3(v2);
	REQUIRE_EQ(v3.GetType(), type);
	CHECK_EQ(*v3.GetPtr<T>(), val);
	REQUIRE_EQ(v2.GetType(), type);
	CHECK_EQ(*v2.GetPtr<T>(), val);

	// Copy assignment
	Variant v4;
	v4 = v3;
	REQUIRE_EQ(v4.GetType(), type);
	CHECK_EQ(*v4.GetPtr<T>(), val);
	REQUIRE_EQ(v3.GetType(), type);
	CHECK_EQ(*v3.GetPtr<T>(), val);

	// Move constructor
	Variant v5(std::move(v4));
	REQUIRE_EQ(v5.GetType(), type);
	CHECK_EQ(*v5.GetPtr<T>(), val);
	CHECK_FALSE(v4); // NOLINT

	// Move assignment
	Variant v6;
	v6 = std::move(v5);
	REQUIRE_EQ(v6.GetType(), type);
	CHECK_EQ(*v6.GetPtr<T>(), val);
	CHECK_FALSE(v5); // NOLINT

	// Destruction
	v6.Clear();
	CHECK_FALSE(v6);
}

TEST_CASE("storage") {
	Object::InitializeClass();
	TestVariant::InitializeClass();

	testVariant(true, Variant::Boolean);
	testVariant<int64_t>(0xDEADBEEF, Variant::Integer);
	testVariant<double>(0.125, Variant::Double);
	testVariant<std::string>("hello world", Variant::String);

	testVariant(Dictionary{ { "a", 1 } }, Variant::Dictionary);
	testVariant(Array{ 1 }, Variant::Array);

	testVariant(*DataTypes::enumAxis.FromName("X"), Variant::EnumItem); // NOLINT

	SUBCASE("function") {
		lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);

		Variant nullFunc = LuauFunction();
		CHECK(!nullFunc);

		EVAL_THEN(L, "return function() end", {
			REQUIRE(lua_isfunction(L, -1));

			Variant v = LuauFunction(L, -1);
			CHECK_EQ(v.GetType(), Variant::Function);
			lua_pop(L, 1);

			LuauFunction *ptr = v.GetPtr<LuauFunction>();
			REQUIRE_NE(ptr, nullptr);

			ptr->Get(L);
			CHECK(lua_isfunction(L, -1));
			lua_pop(L, 1);
		});

		luaSBX_close(L);
	}

	SUBCASE("object") {
		std::shared_ptr<TestVariant> ptr;
		Variant v = ptr;
		CHECK_FALSE(v);

		ptr = std::make_shared<TestVariant>();
		CHECK_EQ(ptr.use_count(), 1);

		v = ptr;
		CHECK_EQ(v.GetType(), Variant::Object);
		CHECK_EQ(ptr.use_count(), 2);

		std::shared_ptr<Object> testBase = v.CastObj<Object>();
		CHECK_EQ(testBase.get(), ptr.get());
		testBase = nullptr;

		std::shared_ptr<Object> testOrig = v.CastObj<TestVariant>();
		CHECK_EQ(testOrig.get(), ptr.get());
		testOrig = nullptr;

		v.Clear();
		CHECK_EQ(ptr.use_count(), 1);
	}
}

TEST_CASE("stack") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);
	Object::InitializeClass();
	TestVariant::InitializeClass();
	ClassDB::Register(L);

	SUBCASE("null") {
		LuauStackOp<Variant>::Push(L, Variant());
		CHECK(lua_isnil(L, -1));

		Variant v = LuauStackOp<Variant>::Get(L, -1);
		CHECK_FALSE(v);
	}

	SUBCASE("bool") {
		LuauStackOp<Variant>::Push(L, true);
		CHECK(lua_isboolean(L, -1));
		CHECK(lua_toboolean(L, -1));

		Variant v = LuauStackOp<Variant>::Get(L, -1);
		auto val = v.Cast<bool>();
		CHECK(val.value_or(false));
	}

	SUBCASE("int") {
		LuauStackOp<Variant>::Push(L, 1234);
		CHECK_EQ(lua_tointeger(L, -1), 1234);

		Variant v = LuauStackOp<Variant>::Get(L, -1);
		auto val = v.Cast<int64_t>();
		CHECK_EQ(val.value_or(0), 1234);
	}

	SUBCASE("double") {
		LuauStackOp<Variant>::Push(L, 0.125);
		CHECK_EQ(lua_tonumber(L, -1), 0.125);

		Variant v = LuauStackOp<Variant>::Get(L, -1);
		auto val = v.Cast<double>();
		CHECK_EQ(val.value_or(0), 0.125);
	}

	SUBCASE("string") {
		LuauStackOp<Variant>::Push(L, "hello world");
		CHECK_EQ(lua_tostring(L, -1), std::string("hello world"));

		Variant v = LuauStackOp<Variant>::Get(L, -1);
		auto val = v.Cast<std::string>();
		CHECK_EQ(val.value_or(""), "hello world");
	}

	SUBCASE("Function") {
		EVAL_THEN(L, "return function() end", {
			int top = lua_gettop(L);
			REQUIRE(lua_isfunction(L, -1));
			LuauStackOp<Variant>::Push(L, LuauFunction(L, -1));
			CHECK_EQ(lua_gettop(L) - top, 1);
			CHECK(lua_isfunction(L, -1));

			Variant v = LuauStackOp<Variant>::Get(L, -1);
			auto val = v.Cast<LuauFunction>();
			REQUIRE(val);
			val->Get(L); // NOLINT
			CHECK_EQ(lua_gettop(L) - top, 2);
			CHECK(lua_isfunction(L, -1));
		});
	}

	SUBCASE("Dictionary") {
		Dictionary testVal{ { "a", 1234.0 }, { "b", "asdf" } };
		LuauStackOp<Variant>::Push(L, testVal);
		REQUIRE(lua_istable(L, -1));

		lua_getfield(L, -1, "a");
		CHECK_EQ(lua_tonumber(L, -1), 1234);
		lua_pop(L, 1);

		lua_getfield(L, -1, "b");
		CHECK_EQ(lua_tostring(L, -1), std::string("asdf"));
		lua_pop(L, 1);

		Variant v = LuauStackOp<Variant>::Get(L, -1);
		auto val = v.Cast<Dictionary>();
		REQUIRE(val);
		CHECK_EQ(*val, testVal); // NOLINT
	}

	SUBCASE("Array") {
		Array testVal{ "abc", 123.0 };
		LuauStackOp<Variant>::Push(L, testVal);
		REQUIRE(lua_istable(L, -1));
		CHECK_EQ(lua_objlen(L, -1), 2);

		lua_rawgeti(L, -1, 1);
		CHECK_EQ(lua_tostring(L, -1), std::string("abc"));
		lua_pop(L, 1);

		lua_rawgeti(L, -1, 2);
		CHECK_EQ(lua_tonumber(L, -1), 123);
		lua_pop(L, 1);

		Variant v = LuauStackOp<Variant>::Get(L, -1);
		auto val = v.Cast<Array>();
		REQUIRE(val);
		CHECK_EQ(*val, testVal); // NOLINT
	}

	SUBCASE("EnumItem") {
		DataTypes::EnumItem *item = *DataTypes::enumAxis.FromName("X"); // NOLINT
		LuauStackOp<Variant>::Push(L, item);
		REQUIRE(LuauStackOp<DataTypes::EnumItem *>::Is(L, -1));
		CHECK_EQ(LuauStackOp<DataTypes::EnumItem *>::Get(L, -1)->GetName(), std::string("X"));

		Variant v = LuauStackOp<Variant>::Get(L, -1);
		auto val = v.Cast<DataTypes::EnumItem *>();
		REQUIRE(val);
		CHECK_EQ(*val, item); // NOLINT
	}

	SUBCASE("Object") {
		std::shared_ptr<TestVariant> obj = std::make_shared<TestVariant>();
		LuauStackOp<Variant>::Push(L, obj);
		REQUIRE(LuauStackOp<std::shared_ptr<TestVariant>>::Is(L, -1));
		CHECK_EQ(LuauStackOp<std::shared_ptr<TestVariant>>::Get(L, -1).get(), obj.get());

		Variant v = LuauStackOp<Variant>::Get(L, -1);
		auto val = v.CastObj<TestVariant>();
		CHECK_EQ(val.get(), obj.get());
	}

	luaSBX_close(L);
}

TEST_SUITE_END();
