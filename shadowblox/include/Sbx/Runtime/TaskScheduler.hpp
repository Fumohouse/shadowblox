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

#include <cstdint>
#include <list>

#include "lua.h"

#include "Sbx/Runtime/Base.hpp"

namespace SBX {

class LuauRuntime;

// Luau uses 1 "step unit" ~= 1KiB
// amount (bytes) = step << 10
#define GC_RATE_MIN 50
#define GC_RATE_INC 25
#define GC_RATE_MAX 10000

// https://create.roblox.com/docs/scripting/events/deferred#resumption-points
enum class ResumptionPoint : uint8_t {
	Input,
	PreRender,
	LegacyWait,
	PreAnimation,
	PreSimulation,
	PostSimulation,
	Wait,
	Heartbeat,
	BindToClose
};

class ScheduledTask {
public:
	ScheduledTask(lua_State *T);
	virtual ~ScheduledTask();

	ScheduledTask(const ScheduledTask &other) = delete;
	ScheduledTask(ScheduledTask &&other) = delete;
	ScheduledTask &operator=(const ScheduledTask &other) = delete;
	ScheduledTask &operator=(ScheduledTask &&other) = delete;

	virtual bool CanThrottle() { return false; }
	virtual int IsComplete(ResumptionPoint) = 0;
	virtual bool ShouldResume() { return true; }
	virtual int PushResults() = 0;
	virtual void Update(uint64_t frame, double delta) = 0;

protected:
	lua_State *T;
	friend class TaskScheduler;

private:
	int threadRef;
};

class TaskScheduler {
public:
	TaskScheduler(LuauRuntime *runtime);

	void AddTask(ScheduledTask *task);

	void Resume(ResumptionPoint point, uint64_t frame, double delta, double throttleThreshold);
	void GCStep(double delta);

	const uint32_t *GetGCStepSize() const { return gcCollectRate; }

private:
	LuauRuntime *runtime;

	std::list<ScheduledTask *> tasks;

	uint32_t gcCollectRate[VMMax] = { GC_RATE_MIN };
	int32_t gcSizeRate[VMMax] = { 0 };
	int32_t gcLastSize[VMMax] = { 0 };
};

void luaSBX_opensched(lua_State *L);

} //namespace SBX
