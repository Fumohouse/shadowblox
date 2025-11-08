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

#include "Sbx/Runtime/TaskScheduler.hpp"

#include <cstdint>
#include <functional>
#include <utility>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/LuauRuntime.hpp"
#include "Sbx/Runtime/SignalEmitter.hpp"

#include "Sbx/Runtime/WaitTask.hpp"

namespace SBX {

ScheduledTask::ScheduledTask(lua_State *T) :
		T(T) {
	// Prevent thread collection while yielded
	lua_pushthread(T);
	threadRef = lua_ref(T, -1);
	lua_pop(T, 1);
}

ScheduledTask::~ScheduledTask() {
	lua_unref(T, threadRef);
}

TaskScheduler::TaskScheduler(LuauRuntime *runtime) :
		runtime(runtime) {
}

void TaskScheduler::Resume(ResumptionPoint point, uint64_t frame, double delta, double throttleThreshold) {
	double start = lua_clock();

	auto it = tasks.begin();
	while (it != tasks.end()) {
		ScheduledTask *task = *it;

		// Do not throttle update (for now) to ensure delta accumulation is
		// correct
		task->Update(frame, delta);

		if ((!task->CanThrottle() || lua_clock() - start < throttleThreshold) &&
				task->IsComplete(point)) {
			if (task->ShouldResume()) {
				int nret = task->PushResults();
				luaSBX_resume(task->GetThread(), nullptr, nret);
			}

			delete *it;

			auto toRemove = it;
			it++;
			tasks.erase(toRemove);

			continue;
		}

		it++;
	}

	// NOTE: Must be careful as this list can be modified during iteration if
	// CancelEvents is called from Disconnect. To avoid memory problems from
	// deleting the DeferredEvent, move each item out of the list and remove
	// the entry before calling resume.
	while (!deferredEvents.empty()) {
		DeferredEvent ev = std::move(deferredEvents.front());
		deferredEvents.pop_front();

		currentReentrancy = std::move(ev.pathReentrancy);
		ev.resume();
	}

	currentReentrancy.clear();
}

void TaskScheduler::GCStep(double delta) {
	int32_t gcNewSize[VMMax];
	runtime->GCSize(gcNewSize);

	for (int i = 0; i < VMMax; i++) {
		gcSizeRate[i] = (int32_t)((gcNewSize[i] - gcLastSize[i]) / delta);
		gcLastSize[i] = gcNewSize[i];
	}

	runtime->GCStep(gcCollectRate, delta);

	for (int i = 0; i < VMMax; i++) {
		uint32_t collRate = gcCollectRate[i];
		int32_t sizeRate = gcSizeRate[i];

		if (sizeRate > 0 && (uint32_t)sizeRate > collRate) {
			// Usage increasing faster than collection
			if (collRate + GC_RATE_INC > GC_RATE_MAX) {
				gcCollectRate[i] = GC_RATE_MAX;
			} else {
				gcCollectRate[i] += GC_RATE_INC;
			}
		} else {
			// Usage increasing slower than collection
			if (collRate < GC_RATE_MIN + GC_RATE_INC) {
				gcCollectRate[i] = GC_RATE_MIN;
			} else {
				gcCollectRate[i] -= GC_RATE_INC;
			}
		}
	}
}

void TaskScheduler::AddTask(ScheduledTask *task) {
	tasks.push_back(task);
}

void TaskScheduler::AddDeferredEvent(const char *name, SignalEmitter *emitter, uint64_t id, lua_State *L, std::function<void()> resume) {
	if (currentReentrancy[emitter][id] >= DEFERRED_EVENT_REENTRANCY_LIMIT) {
		// Function must be on top of stack
		luaSBX_reentrancyerror(L, name);
	} else {
		lua_pop(L, 1); // function

		auto childReentrancy = currentReentrancy;
		childReentrancy[emitter][id]++;

		deferredEvents.push_back({ emitter,
				id,
				L,
				std::move(resume),
				std::move(childReentrancy) });
	}
}

void TaskScheduler::CancelTask(ScheduledTask *task) {
	delete task;
	tasks.remove(task);
}

void TaskScheduler::CancelThread(lua_State *L) {
	// May be relevant in case of a destroyed Actor
	tasks.remove_if([=](ScheduledTask *task) {
		if (task->GetThread() == L) {
			delete task;
			return true;
		}

		return false;
	});

	deferredEvents.remove_if([=](const DeferredEvent &ev) {
		return ev.L == L;
	});
}

void TaskScheduler::CancelEvents(SignalEmitter *emitter, uint64_t id) {
	deferredEvents.remove_if([=](DeferredEvent &ev) {
		return ev.emitter == emitter && ev.id == id;
	});
}

int TaskScheduler::NumPendingTasks() const {
	return tasks.size();
}

int TaskScheduler::NumPendingEvents() const {
	return deferredEvents.size();
}

static const luaL_Reg LEGACY_SCHEDULER_LIB[] = {
	{ "wait", luaSBX_wait },

	{ nullptr, nullptr }
};

static const luaL_Reg SCHEDULER_LIB[] = {
	{ "wait", luaSBX_taskwait },

	{ nullptr, nullptr }
};

void luaSBX_opensched(lua_State *L) {
	luaL_register(L, "_G", LEGACY_SCHEDULER_LIB);
	luaL_register(L, "task", SCHEDULER_LIB);
	lua_pop(L, 2); // tables
}

} //namespace SBX
