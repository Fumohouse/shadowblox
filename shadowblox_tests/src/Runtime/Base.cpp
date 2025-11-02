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

#include "lua.h"

#include "Sbx/Runtime/Base.hpp"
#include "Utils.hpp"

using namespace SBX;

TEST_SUITE_BEGIN("Runtime/Base");

TEST_CASE("newstate") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	REQUIRE_NE(L, nullptr);
	CHECK_EQ(lua_gettop(L), 0);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	CHECK_EQ(udata->vmType, CoreVM);
	CHECK_EQ(udata->identity, ElevatedGameScriptIdentity);

	luaSBX_close(L);
}

TEST_CASE("newthread") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	REQUIRE_NE(L, nullptr);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	REQUIRE_NE(udata, nullptr);

	SUBCASE("lua_newthread") {
		lua_State *T = lua_newthread(L);
		REQUIRE_NE(T, nullptr);

		SbxThreadData *threadUdata = luaSBX_getthreaddata(T);
		REQUIRE_NE(threadUdata, nullptr);

		CHECK_EQ(threadUdata->vmType, udata->vmType);
		CHECK_EQ(threadUdata->identity, udata->identity);
		CHECK_EQ(&*threadUdata->mutex, &*udata->mutex);

		lua_pop(L, 1);
	}

	SUBCASE("luaSBX_newthread") {
		lua_State *T = luaSBX_newthread(L, AnonymousIdentity);
		REQUIRE_NE(T, nullptr);

		SbxThreadData *threadUdata = luaSBX_getthreaddata(T);
		REQUIRE_NE(threadUdata, nullptr);

		CHECK_EQ(threadUdata->vmType, udata->vmType);
		CHECK_EQ(threadUdata->identity, AnonymousIdentity);
		CHECK_EQ(&*threadUdata->mutex, &*udata->mutex);

		lua_pop(L, 1);
	}

	luaSBX_close(L);
}

TEST_CASE("permissions") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	REQUIRE_NE(L, nullptr);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	REQUIRE_NE(udata, nullptr);
	CHECK(luaSBX_iscapability(L, RobloxScriptSecurity));
	CHECK_FALSE(luaSBX_iscapability(L, NotAccessibleSecurity));

	udata->identity = GameScriptIdentity;
	CHECK_FALSE(luaSBX_iscapability(L, RobloxScriptSecurity));

	udata->additionalCapability = RobloxScriptSecurity;
	CHECK(luaSBX_iscapability(L, RobloxScriptSecurity));
	udata->additionalCapability = NoneSecurity;

	udata->identity = COMIdentity;
	CHECK(luaSBX_iscapability(L, NotAccessibleSecurity));

	luaSBX_close(L);
}

TEST_CASE("registry") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	REQUIRE_NE(L, nullptr);

	auto push = [](lua_State *L, void *ptr) {
		lua_newuserdata(L, 1);
	};

	SUBCASE("strong") {
		CHECK(luaSBX_pushregistry(L, nullptr, push, false));
		CHECK_FALSE(luaSBX_pushregistry(L, nullptr, push, false));
		CHECK_EQ(lua_gettop(L), 2);

		CHECK(lua_equal(L, -2, -1));
		lua_pop(L, 2);
		lua_gc(L, LUA_GCCOLLECT, 0);

		CHECK_FALSE(luaSBX_pushregistry(L, nullptr, push, false));
		lua_pop(L, 1);
	}

	SUBCASE("weak") {
		CHECK(luaSBX_pushregistry(L, nullptr, push, true));
		CHECK_FALSE(luaSBX_pushregistry(L, nullptr, push, true));
		CHECK_EQ(lua_gettop(L), 2);

		CHECK(lua_equal(L, -2, -1));
		lua_pop(L, 2);
		lua_gc(L, LUA_GCCOLLECT, 0);

		CHECK(luaSBX_pushregistry(L, nullptr, push, false));
		lua_pop(L, 1);
	}

	luaSBX_close(L);
}

TEST_CASE("timeout") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	luaSBX_debugcallbacks(L);

	CHECK_EVAL_FAIL(L, "while true do end", "Script timed out: exhausted allowed execution time");

	luaSBX_close(L);
}

TEST_SUITE_END();
