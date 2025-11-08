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

#include <memory>
#include <string>

#include "lua.h"

#include "Sbx/DataTypes/RBXScriptConnection.hpp"
#include "Sbx/DataTypes/RBXScriptSignal.hpp"
#include "Sbx/DataTypes/Types.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Logger.hpp"
#include "Sbx/Runtime/SignalEmitter.hpp"
#include "Sbx/Runtime/TaskScheduler.hpp"
#include "Utils.hpp"

using namespace SBX;
using namespace SBX::DataTypes;

TEST_SUITE_BEGIN("Runtime/SignalEmitter");

TEST_CASE("immediate") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	luaSBX_opendatatypes(L);
	static std::shared_ptr<SignalEmitter> emitter;
	emitter = std::make_shared<SignalEmitter>("TestEmitter", false);
	Logger logger;
	SignalConnectionOwner connections;

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->global->logger = &logger;
	udata->signalConnections = &connections;

	SUBCASE("basic functionality") {
		RBXScriptSignal signal(emitter, "TestSignal");
		LuauStackOp<RBXScriptSignal>::Push(L, signal);
		lua_setglobal(L, "signal");

		CHECK_EVAL_OK(L, R"ASDF(
			hits = 0
			onceHits = 0
			arg1 = nil
			arg2 = nil

			local conn

			conn = signal:Connect(function(a1, a2)
				hits += 1
				arg1 = a1
				arg2 = a2

				if hits == 2 then conn:Disconnect() end
			end)

			signal:Once(function()
				onceHits += 1
			end)
		)ASDF")

		emitter->Emit("TestSignal", "abc", 123);
		emitter->Emit("TestSignal", "def", 456);
		emitter->Emit("TestSignal", "ghi", 789);

		lua_getglobal(L, "hits");
		CHECK_EQ(lua_tonumber(L, -1), 2);
		lua_pop(L, 1);

		lua_getglobal(L, "onceHits");
		CHECK_EQ(lua_tonumber(L, -1), 1);
		lua_pop(L, 1);

		lua_getglobal(L, "arg1");
		CHECK_EQ(lua_tostring(L, -1), std::string("def"));
		lua_pop(L, 1);

		lua_getglobal(L, "arg2");
		CHECK_EQ(lua_tonumber(L, -1), 456);
		lua_pop(L, 1);
	}

	SUBCASE("reentrancy negative") {
		RBXScriptSignal signal1(emitter, "TestSignal1");
		LuauStackOp<RBXScriptSignal>::Push(L, signal1);
		lua_setglobal(L, "signal1");

		RBXScriptSignal signal2(emitter, "TestSignal2");
		LuauStackOp<RBXScriptSignal>::Push(L, signal2);
		lua_setglobal(L, "signal2");

		lua_pushcfunction(L, [](lua_State *) -> int {
			emitter->Emit("TestSignal2");
			return 0; }, "emit2");
		lua_setglobal(L, "emit2");

		CHECK_EVAL_OK(L, R"ASDF(
			hits2 = 0

			signal1:Connect(function()
				for i = 1, 8 do
					emit2()
				end
			end)

			signal2:Connect(function()
				hits2 += 1
			end)
		)ASDF")

		emitter->Emit("TestSignal1");

		lua_getglobal(L, "hits2");
		CHECK_EQ(lua_tonumber(L, -1), 8);
		lua_pop(L, 1);
	}

	SUBCASE("reentrancy positive") {
		RBXScriptSignal signal(emitter, "TestSignal");
		LuauStackOp<RBXScriptSignal>::Push(L, signal);
		lua_setglobal(L, "signal");

		lua_pushcfunction(L, [](lua_State *) -> int {
			emitter->Emit("TestSignal");
			return 0; }, "emit");
		lua_setglobal(L, "emit");

		CHECK_EVAL_OK(L, R"ASDF(
			hits = 0

			signal:Connect(function()
				hits += 1
				emit()
			end)
		)ASDF")

		emitter->Emit("TestSignal");

		lua_getglobal(L, "hits");
		CHECK_EQ(lua_tonumber(L, -1), 6);
		lua_pop(L, 1);
	}

	luaSBX_close(L);
	CHECK_EQ(emitter->NumConnections(), 0);
	emitter = nullptr;
}

TEST_CASE("deferred") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	luaSBX_opendatatypes(L);
	static std::shared_ptr<SignalEmitter> emitter;
	emitter = std::make_shared<SignalEmitter>("TestEmitter", true);
	TaskScheduler scheduler(nullptr);
	Logger logger;
	SignalConnectionOwner connections;

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->global->scheduler = &scheduler;
	udata->global->logger = &logger;
	udata->signalConnections = &connections;

	SUBCASE("basic functionality") {
		RBXScriptSignal signal(emitter, "TestSignal");
		LuauStackOp<RBXScriptSignal>::Push(L, signal);
		lua_setglobal(L, "signal");

		CHECK_EVAL_OK(L, R"ASDF(
			hits = 0
			onceHits = 0
			arg1 = nil
			arg2 = nil

			local conn

			conn = signal:Connect(function(a1, a2)
				hits += 1
				arg1 = a1
				arg2 = a2

				if hits == 2 then conn:Disconnect() end
			end)

			signal:Once(function()
				onceHits += 1
			end)
		)ASDF")

		emitter->Emit("TestSignal", "abc", 123);
		emitter->Emit("TestSignal", "def", 456);
		// This resumption should be queued but canceled by Disconnect
		emitter->Emit("TestSignal", "ghi", 789);

		CHECK_EQ(scheduler.NumPendingEvents(), 4);

		lua_getglobal(L, "hits");
		CHECK_EQ(lua_tonumber(L, -1), 0);
		lua_pop(L, 1);

		lua_getglobal(L, "onceHits");
		CHECK_EQ(lua_tonumber(L, -1), 0);
		lua_pop(L, 1);

		scheduler.Resume(ResumptionPoint::Heartbeat, 1, 1.0, 1.0);

		lua_getglobal(L, "hits");
		CHECK_EQ(lua_tonumber(L, -1), 2);
		lua_pop(L, 1);

		lua_getglobal(L, "onceHits");
		CHECK_EQ(lua_tonumber(L, -1), 1);
		lua_pop(L, 1);

		lua_getglobal(L, "arg1");
		CHECK_EQ(lua_tostring(L, -1), std::string("def"));
		lua_pop(L, 1);

		lua_getglobal(L, "arg2");
		CHECK_EQ(lua_tonumber(L, -1), 456);
		lua_pop(L, 1);

		luaSBX_close(L);
		CHECK_EQ(emitter->NumConnections(), 0);
		emitter = nullptr;
	}

	SUBCASE("reentrancy negative") {
		RBXScriptSignal signal1(emitter, "TestSignal1");
		LuauStackOp<RBXScriptSignal>::Push(L, signal1);
		lua_setglobal(L, "signal1");

		RBXScriptSignal signal2(emitter, "TestSignal2");
		LuauStackOp<RBXScriptSignal>::Push(L, signal2);
		lua_setglobal(L, "signal2");

		lua_pushcfunction(L, [](lua_State *) -> int {
		emitter->Emit("TestSignal2");
		return 0; }, "emit2");
		lua_setglobal(L, "emit2");

		CHECK_EVAL_OK(L, R"ASDF(
			hits2 = 0

			signal1:Connect(function()
				for i = 1, 16 do
					emit2()
				end
			end)

			signal2:Connect(function()
				hits2 += 1
			end)
		)ASDF")

		for (int i = 0; i < 16; i++) {
			emitter->Emit("TestSignal1");
		}

		scheduler.Resume(ResumptionPoint::Heartbeat, 1, 1.0, 1.0);

		lua_getglobal(L, "hits2");
		CHECK_EQ(lua_tonumber(L, -1), 256);
		lua_pop(L, 1);

		luaSBX_close(L);
		CHECK_EQ(emitter->NumConnections(), 0);
		emitter = nullptr;
	}

	SUBCASE("reentrancy positive") {
		RBXScriptSignal signal(emitter, "TestSignal");
		LuauStackOp<RBXScriptSignal>::Push(L, signal);
		lua_setglobal(L, "signal");

		lua_pushcfunction(L, [](lua_State *) -> int {
		emitter->Emit("TestSignal");
		return 0; }, "emit");
		lua_setglobal(L, "emit");

		CHECK_EVAL_OK(L, R"ASDF(
			hits = 0

			signal:Connect(function()
				hits += 1
				emit()
			end)
		)ASDF")

		emitter->Emit("TestSignal");

		scheduler.Resume(ResumptionPoint::Heartbeat, 1, 1.0, 1.0);

		lua_getglobal(L, "hits");
		CHECK_EQ(lua_tonumber(L, -1), 79);
		lua_pop(L, 1);

		luaSBX_close(L);
		CHECK_EQ(emitter->NumConnections(), 0);
		emitter = nullptr;
	}

	SUBCASE("after emitter free") {
		LuauStackOp<RBXScriptSignal>::Push(L, RBXScriptSignal(emitter, "TestSignal"));
		lua_setglobal(L, "signal");

		CHECK_EVAL_OK(L, R"ASDF(
			hits = 0

			signal:Connect(function()
				hits += 1
			end)
		)ASDF")

		lua_pushnil(L);
		lua_setglobal(L, "signal");
		lua_gc(L, LUA_GCCOLLECT, 0);

		emitter->Emit("TestSignal");

		lua_getglobal(L, "hits");
		CHECK_EQ(lua_tonumber(L, -1), 0);
		lua_pop(L, 1);

		REQUIRE_EQ(emitter.use_count(), 1);
		emitter = nullptr;

		scheduler.Resume(ResumptionPoint::Heartbeat, 1, 1.0, 1.0);

		lua_getglobal(L, "hits");
		CHECK_EQ(lua_tonumber(L, -1), 1);
		lua_pop(L, 1);

		luaSBX_close(L);
	}
}

TEST_CASE("wait") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	luaSBX_opendatatypes(L);
	static std::shared_ptr<SignalEmitter> emitter;
	emitter = std::make_shared<SignalEmitter>("TestEmitter", true);
	TaskScheduler scheduler(nullptr);
	Logger logger;
	SignalConnectionOwner connections;

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->global->scheduler = &scheduler;
	udata->global->logger = &logger;
	udata->signalConnections = &connections;

	RBXScriptSignal signal(emitter, "TestSignal");
	LuauStackOp<RBXScriptSignal>::Push(L, signal);
	lua_setglobal(L, "signal");

	CHECK_EVAL_OK(L, "local a, b = signal:Wait(); return a, b");
	REQUIRE_EQ(lua_status(L), LUA_YIELD);

	scheduler.Resume(ResumptionPoint::Heartbeat, 1, 1.0, 1.0);
	REQUIRE_EQ(lua_status(L), LUA_YIELD);

	emitter->Emit("TestSignal", "abc", 123);
	scheduler.Resume(ResumptionPoint::Heartbeat, 1, 1.0, 1.0);
	REQUIRE_EQ(lua_status(L), LUA_OK);

	REQUIRE_EQ(lua_gettop(L), 2);
	CHECK_EQ(lua_tostring(L, 1), std::string("abc"));
	CHECK_EQ(lua_tonumber(L, 2), 123);

	luaSBX_close(L);
	CHECK_EQ(emitter->NumConnections(), 0);
	emitter = nullptr;
}

TEST_CASE("Luau API") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	luaSBX_opendatatypes(L);
	std::shared_ptr<SignalEmitter> emitter = std::make_shared<SignalEmitter>("TestEmitter", false);
	SignalConnectionOwner connections;

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->signalConnections = &connections;

	SUBCASE("signal equality") {
		RBXScriptSignal signal(emitter, "TestSignal");
		LuauStackOp<RBXScriptSignal>::Push(L, signal);
		lua_setglobal(L, "signal1");
		LuauStackOp<RBXScriptSignal>::Push(L, signal);
		lua_setglobal(L, "signal2");

		CHECK_EVAL_EQ(L, "return rawequal(signal1, signal2)", bool, false);
		CHECK_EVAL_EQ(L, "return signal1 == signal2", bool, true);
	}

	SUBCASE("signal tostring") {
		RBXScriptSignal signal(emitter, "TestSignal");
		LuauStackOp<RBXScriptSignal>::Push(L, signal);
		lua_setglobal(L, "signal");

		CHECK_EVAL_EQ(L, "return tostring(signal)", std::string, "Signal TestSignal");
	}

	SUBCASE("connection tostring") {
		RBXScriptSignal signal(emitter, "TestSignal");
		LuauStackOp<RBXScriptSignal>::Push(L, signal);
		lua_setglobal(L, "signal");

		CHECK_EVAL_EQ(L, "return tostring(signal:Connect(function() end))", std::string, "Connection");
	}

	luaSBX_close(L);
}

TEST_SUITE_END();
