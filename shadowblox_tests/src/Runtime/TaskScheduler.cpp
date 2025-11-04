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
#include "Sbx/Runtime/TaskScheduler.hpp"
#include "Utils.hpp"

using namespace SBX;

TEST_SUITE_BEGIN("Runtime/TaskScheduler");

TEST_CASE("resume") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	TaskScheduler scheduler(nullptr);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->global->scheduler = &scheduler;

	SUBCASE("legacy wait") {
		CHECK_EVAL_OK(L, R"ASDF(
			local t, u = wait(1)
			assert(t > 0)
			assert(u > 0)
		)ASDF")

		REQUIRE_EQ(lua_status(L), LUA_YIELD);

		// Throttle
		scheduler.Resume(ResumptionPoint::Heartbeat, 1, 0.5, -1.0);
		REQUIRE_EQ(lua_status(L), LUA_YIELD);

		// Pass
		scheduler.Resume(ResumptionPoint::Heartbeat, 2, 0.6, 1.0);
		REQUIRE_EQ(lua_status(L), LUA_OK);
	}

	luaSBX_close(L);
}

TEST_SUITE_END();
