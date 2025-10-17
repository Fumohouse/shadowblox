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

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"
#include "lua.h"

TEST_SUITE_BEGIN("Runtime/LuauRuntime");

TEST_CASE("initialize") {
	static int hit = 0;
	LuauRuntime rt([](lua_State *L) {
		hit++;
	});

	CHECK_EQ(hit, LuauRuntime::VMMax);

	auto LC = rt.getVM(LuauRuntime::CoreVM);
	SbxThreadData *udataCore = luaSBX_getthreaddata(LC);
	REQUIRE_NE(udataCore, nullptr);
	CHECK_EQ(udataCore->vmType, LuauRuntime::CoreVM);
	CHECK_EQ(udataCore->identity, ElevatedGameScriptIdentity);

	auto LU = rt.getVM(LuauRuntime::UserVM);
	SbxThreadData *udataUser = luaSBX_getthreaddata(LU);
	REQUIRE_NE(udataUser, nullptr);
	CHECK_EQ(udataUser->vmType, LuauRuntime::UserVM);
	CHECK_EQ(udataUser->identity, GameScriptIdentity);

	// Ensure acquisition of mutex
	CHECK_FALSE(udataCore->mutex->try_lock());
	CHECK_FALSE(udataUser->mutex->try_lock());
}

TEST_SUITE_END();
