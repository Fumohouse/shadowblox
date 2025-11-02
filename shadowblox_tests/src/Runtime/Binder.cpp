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

#include <string>
#include <tuple>

#include "lua.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Binder.hpp"
#include "Sbx/Runtime/Stack.hpp"
#include "Utils.hpp"

using namespace SBX;

TEST_SUITE_BEGIN("Runtime/Binder");

int testRet(int x, int y) {
	return x * y;
}

void testVoid(float a, lua_State *L, float b) {
	CHECK_EQ(a, 0.5);
	CHECK_EQ(b, 0.25);
	CHECK_EQ(lua_gettop(L), 2);
}

std::tuple<int, bool, float, const char *> testTuple() {
	return { 1, true, 0.25, "Hello" };
}

TEST_CASE("static") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	SbxThreadData *udata = luaSBX_getthreaddata(L);

	lua_pushcfunction(L, (luaSBX_bindcxx<"testRet", testRet, InternalTestSecurity>), "testRet");
	lua_setglobal(L, "testRet");

	lua_pushcfunction(L, (luaSBX_bindcxx<"testVoid", testVoid, InternalTestSecurity>), "testVoid");
	lua_setglobal(L, "testVoid");

	lua_pushcfunction(L, (luaSBX_bindcxx<"testTuple", testTuple, InternalTestSecurity>), "testTuple");
	lua_setglobal(L, "testTuple");

	SUBCASE("working") {
		CHECK_EVAL_EQ(L, "return testRet(12, 5)", int, 60);
		CHECK_EVAL_EQ(L, "return testRet(6, 8.2)", int, 48);

		CHECK_EVAL_OK(L, "testVoid(0.5, 0.25)");

		EVAL_THEN(L, "return testTuple()", {
			REQUIRE_EQ(lua_gettop(L), 4);
			REQUIRE(lua_isnumber(L, 1));
			REQUIRE(lua_isboolean(L, 2));
			REQUIRE(lua_isnumber(L, 3));
			REQUIRE_EQ(lua_type(L, 4), LUA_TSTRING);

			CHECK_EQ(lua_tonumber(L, 1), 1);
			CHECK_EQ(lua_toboolean(L, 2), true);
			CHECK_EQ(lua_tonumber(L, 3), 0.25);
			CHECK_EQ(std::string(lua_tostring(L, 4)), "Hello");
		});
	}

	SUBCASE("missing arg") {
		CHECK_EVAL_FAIL(L, "return testRet()", "exec:1: Argument 1 missing or nil");
	}

	SUBCASE("wrong arg") {
		CHECK_EVAL_FAIL(L, "testVoid('', nil)", "exec:1: Unable to cast string to float");
	}

	SUBCASE("missing permission") {
		udata->identity = GameScriptIdentity;
		CHECK_EVAL_FAIL(L, "testVoid()", "exec:1: The current identity (2) cannot call 'testVoid' (lacking permission 17)");
	}

	luaSBX_close(L);
}

struct TestClass {
	int z = 0;

	int TestFuncConst(int x, int y) const {
		return x * y;
	}

	int TestFuncNonConst(int x) {
		z = x;
		return z * 2;
	}

	void TestFuncVoid() {
		z = 123;
	}
};

namespace SBX {

STACK_OP_STATIC_PTR_DEF(TestClass);
STATIC_PTR_STACK_OP_IMPL(TestClass, "TestClass", "SbxTests.TestClass", Test1Udata);

} //namespace SBX

TEST_CASE("class") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	SbxThreadData *udata = luaSBX_getthreaddata(L);

	lua_newtable(L);
	lua_setreadonly(L, -1, true);
	lua_setuserdatametatable(L, Test1Udata);

	lua_pushcfunction(L, (luaSBX_bindcxx<"TestFuncConst", &TestClass::TestFuncConst, InternalTestSecurity>), "TestFuncConst");
	lua_setglobal(L, "TestFuncConst");

	lua_pushcfunction(L, (luaSBX_bindcxx<"TestFuncNonConst", &TestClass::TestFuncNonConst, InternalTestSecurity>), "TestFuncNonConst");
	lua_setglobal(L, "TestFuncNonConst");

	lua_pushcfunction(L, (luaSBX_bindcxx<"TestFuncVoid", &TestClass::TestFuncVoid, InternalTestSecurity>), "TestFuncVoid");
	lua_setglobal(L, "TestFuncVoid");

	TestClass inst;
	LuauStackOp<TestClass *>::Push(L, &inst);
	lua_setglobal(L, "inst");

	SUBCASE("working") {
		CHECK_EVAL_EQ(L, "return TestFuncConst(inst, 4, 8)", int, 32);
		CHECK_EVAL_EQ(L, "return TestFuncNonConst(inst, 21)", int, 42);

		CHECK_EVAL_OK(L, "TestFuncVoid(inst)");
		CHECK_EQ(inst.z, 123);
	}

	SUBCASE("invalid instance") {
		CHECK_EVAL_FAIL(L, "TestFuncConst('a')", "exec:1: Expected ':' not '.' calling member function TestFuncConst");
	}

	SUBCASE("invalid arg") {
		CHECK_EVAL_FAIL(L, "TestFuncConst(inst, 1, nil)", "exec:1: Argument 2 missing or nil");
	}

	SUBCASE("missing permission") {
		udata->identity = GameScriptIdentity;
		CHECK_EVAL_FAIL(L, "TestFuncConst()", "exec:1: The current identity (2) cannot call 'TestFuncConst' (lacking permission 17)");
	}

	luaSBX_close(L);
}

TEST_SUITE_END();
