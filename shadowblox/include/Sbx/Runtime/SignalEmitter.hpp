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
#include <functional>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "lua.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Stack.hpp"
#include "Sbx/Runtime/TaskScheduler.hpp"

namespace SBX {

#define IMMEDIATE_EVENT_REENTRANCY_LIMIT 6
#define DEFERRED_EVENT_REENTRANCY_LIMIT 79

void luaSBX_reentrancyerror(lua_State *L, const char *signalName);

class SignalWaitTask : public ScheduledTask {
public:
	using ScheduledTask::ScheduledTask;

	bool IsComplete(ResumptionPoint point) override;
	int PushResults() override;

private:
	friend class SignalEmitter;
	std::function<int(lua_State *)> pushResults;
};

class SignalEmitter {
public:
	SignalEmitter();
	~SignalEmitter();

	SignalEmitter(const SignalEmitter &other) = delete;
	SignalEmitter(SignalEmitter &&other) = delete;
	SignalEmitter &operator=(const SignalEmitter &other) = delete;
	SignalEmitter &operator=(SignalEmitter &&other) = delete;

	void SetDeferred(bool isDeferred);

	uint64_t Connect(const std::string &signal, lua_State *L, bool once);
	bool IsConnected(const std::string &signal, uint64_t id) const;
	void Disconnect(const std::string &signal, uint64_t id, bool cancel = true, bool updateOwner = true);
	int Wait(const std::string &signal, lua_State *L);

	int NumConnections() const;

	template <typename... Args>
	void Emit(const std::string &signal, const char *debugName, Args... args) {
		std::vector<uint64_t> toRemove;

		if (deferred) {
			// Do not fire new connections during iteration
			auto connectionsCopy = connections[signal];
			for (const auto &[id, conn] : connectionsCopy) {
				SbxThreadData *udata = luaSBX_getthreaddata(conn.L);
				if (!udata->global->scheduler) {
					continue;
				}

				// Duplicate this reference: If this object is collected during
				// the resumption period, then the original conn.ref will be
				// destroyed before the resumption of the event handler.
				lua_getref(conn.L, conn.ref);
				int newRef = lua_ref(conn.L, -1);
				// Leave function on stack for AddDeferredEvent

				udata->global->scheduler->AddDeferredEvent(debugName, this, id, conn.L, [=]() {
					lua_getref(conn.L, newRef);
					(LuauStackOp<Args>::Push(conn.L, args), ...);
					luaSBX_pcall(conn.L, sizeof...(Args), 0, 0);
					lua_unref(conn.L, newRef);
				});

				if (conn.once) {
					toRemove.push_back(id);
				}
			}
		} else {
			// Do not fire new connections during iteration
			auto connectionsCopy = connections[signal];
			for (const auto &[id, conn] : connectionsCopy) {
				// Deleted during iteration
				if (!connections[signal].contains(id)) {
					continue;
				}

				lua_getref(conn.L, conn.ref);

				bool firstEntrant = immediateReentrancy.empty();

				if (++immediateReentrancy[id] > IMMEDIATE_EVENT_REENTRANCY_LIMIT) {
					luaSBX_reentrancyerror(conn.L, debugName);
				} else {
					(LuauStackOp<Args>::Push(conn.L, args), ...);
					luaSBX_pcall(conn.L, sizeof...(Args), 0, 0);
				}

				immediateReentrancy[id]--;
				if (firstEntrant) {
					immediateReentrancy.clear();
				}

				if (conn.once) {
					toRemove.push_back(id);
				}
			}
		}

		for (uint64_t id : toRemove) {
			Disconnect(signal, id, false);
		}

		auto tasks = pendingTasks.find(signal);
		if (tasks != pendingTasks.end()) {
			for (SignalWaitTask *task : tasks->second) {
				task->pushResults = [=](lua_State *L) -> int {
					(LuauStackOp<Args>::Push(L, args), ...);
					return sizeof...(Args);
				};
			}

			tasks->second.clear();
		}
	}

private:
	struct Connection {
		lua_State *L;
		int ref;
		bool once;
	};

	bool deferred = false;
	uint64_t nextId = 0;
	std::unordered_map<uint64_t, int> immediateReentrancy;
	std::unordered_map<std::string, std::unordered_map<uint64_t, Connection>> connections;
	std::unordered_map<std::string, std::list<SignalWaitTask *>> pendingTasks;
};

class SignalConnectionOwner {
public:
	void Clear();
	void ClearEmitter(SignalEmitter *emitter);

protected:
	friend class SignalEmitter;
	void AddConnection(SignalEmitter *emitter, const std::string &name, uint64_t id);
	void RemoveConnection(SignalEmitter *emitter, uint64_t id);

private:
	std::unordered_map<SignalEmitter *, std::unordered_map<uint64_t, std::string>> connections;
};

} //namespace SBX
