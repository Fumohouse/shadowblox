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
#include <optional>
#include <string>

#include "lua.h"
#include "lualib.h" // NOLINT

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"
#include "Sbx/Runtime/Stack.hpp"

using namespace SBX;

TEST_SUITE_BEGIN("Runtime/Stack");

struct TestStruct {
	int value = 0;

	bool operator==(const TestStruct &other) const {
		return value == other.value;
	}
};

STACK_OP_UDATA_DEF(TestStruct);
UDATA_STACK_OP_IMPL(TestStruct, "SbxTests.TestStruct", Test1Udata, NO_DTOR);

template <typename T, typename U>
static inline void testStackOp(lua_State *L, U value) {
	int top = lua_gettop(L);

	LuauStackOp<T>::Push(L, value);
	CHECK_EQ(lua_gettop(L), top + 1);

	CHECK(LuauStackOp<T>::Is(L, -1));
	CHECK_EQ(lua_gettop(L), top + 1);

	CHECK_EQ(LuauStackOp<T>::Get(L, -1), value);
	CHECK_EQ(lua_gettop(L), top + 1);

	CHECK_EQ(LuauStackOp<T>::Check(L, -1), value);
	CHECK_EQ(lua_gettop(L), top + 1);

	CHECK(LuauStackOp<std::optional<T>>::Is(L, -1));
	CHECK_EQ(lua_gettop(L), top + 1);

	CHECK_EQ(*LuauStackOp<std::optional<T>>::Get(L, -1), value);
	CHECK_EQ(lua_gettop(L), top + 1);

	lua_pop(L, 1);

	lua_pushnil(L);

	CHECK_FALSE(LuauStackOp<T>::Is(L, -1));
	CHECK_EQ(lua_gettop(L), top + 1);

	CHECK(LuauStackOp<std::optional<T>>::Is(L, -1));
	CHECK_EQ(lua_gettop(L), top + 1);

	CHECK_FALSE(LuauStackOp<std::optional<T>>::Get(L, -1).has_value());
	CHECK_EQ(lua_gettop(L), top + 1);

	lua_pop(L, 1);
}

TEST_CASE("basic") {
	lua_State *L = luaSBX_newstate(LuauRuntime::CoreVM, AnonymousIdentity);

	testStackOp<bool>(L, true);
	testStackOp<int>(L, 12);
	testStackOp<std::string>(L, "hello there! おはようございます");

	luaSBX_close(L);
}

TEST_CASE("int64") {
	lua_State *L = luaSBX_newstate(LuauRuntime::CoreVM, AnonymousIdentity);

	SUBCASE("|x| <= 2^53") {
		int64_t i = 9007199254740992;
		testStackOp<int64_t>(L, i);

		LuauStackOp<int64_t>::Push(L, i);
		REQUIRE(lua_isnumber(L, -1));
		lua_pop(L, 1);
	}

	SUBCASE("|x| > 2^53") {
		int64_t j = 9007199254740993;
		testStackOp<int64_t>(L, j);

		LuauStackOp<int64_t>::Push(L, j);
		REQUIRE(lua_isuserdata(L, -1));
		lua_pop(L, 1);
	}

	luaSBX_close(L);
}

TEST_CASE("udata") {
	lua_State *L = luaSBX_newstate(LuauRuntime::CoreVM, AnonymousIdentity);

	lua_newtable(L);
	lua_setreadonly(L, -1, true);
	lua_setuserdatametatable(L, Test1Udata);

	TestStruct x{ 42 };
	testStackOp<TestStruct>(L, x);

	SUBCASE("pointer") {
		LuauStackOp<TestStruct *>::Push(L, &x);

		CHECK(LuauStackOp<TestStruct *>::Is(L, -1));

		TestStruct *ptr = LuauStackOp<TestStruct *>::Get(L, -1);
		TestStruct *checkPtr = LuauStackOp<TestStruct *>::Check(L, -1);
		REQUIRE_NE(ptr, nullptr);
		CHECK_EQ(ptr, checkPtr);
		CHECK_EQ(ptr->value, 42);
	}

	SUBCASE("const pointer") {
		LuauStackOp<const TestStruct *>::Push(L, &x);

		CHECK(LuauStackOp<const TestStruct *>::Is(L, -1));

		const TestStruct *ptr = LuauStackOp<const TestStruct *>::Get(L, -1);
		const TestStruct *checkPtr = LuauStackOp<const TestStruct *>::Check(L, -1);
		REQUIRE_NE(ptr, nullptr);
		CHECK_EQ(ptr, checkPtr);
		CHECK_EQ(ptr->value, 42);
	}

	SUBCASE("reference") {
		LuauStackOp<TestStruct &>::Push(L, x);

		CHECK_EQ(LuauStackOp<const TestStruct &>::Check(L, -1).value, 42);
		CHECK_EQ(LuauStackOp<const TestStruct &>::Get(L, -1).value, 42);
		CHECK_EQ(LuauStackOp<TestStruct &>::Check(L, -1).value, 42);
		CHECK_EQ(LuauStackOp<TestStruct &>::Get(L, -1).value, 42);
	}

	lua_pop(L, 1);

	luaSBX_close(L);
}

TEST_SUITE_END();
