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

#include "Sbx/Runtime/SignalEmitter.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

#include "lua.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Logger.hpp"
#include "Sbx/Runtime/TaskScheduler.hpp"

namespace SBX {

void luaSBX_reentrancyerror(lua_State *L, const char *signalName) {
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	if (!udata->global->logger)
		return;

	lua_Debug ar;
	// -1: Function on top of stack
	lua_getinfo(L, -1, "sn", &ar);
	// Skip first character of ar.source (should be @ or =)
	udata->global->logger->Error(
			"Maximum event re-entrancy depth exceeded for %s when calling %s on line %d in %s",
			signalName, ar.name ? ar.name : "anonymous function", ar.linedefined, ar.source + 1);

	lua_pop(L, 1);
}

SignalEmitter::SignalEmitter(std::string name, bool deferred) :
		name(std::move(name)), deferred(deferred) {
}

SignalEmitter::~SignalEmitter() {
	for (const auto &[connName, conns] : connections) {
		for (const auto &[id, conn] : conns) {
			SbxThreadData *udata = luaSBX_getthreaddata(conn.L);
			if (udata->signalConnections) {
				udata->signalConnections->ClearEmitter(this);
			}

			lua_unref(conn.L, conn.ref);
		}
	}

	for (const auto &[name, tasks] : pendingTasks) {
		for (SignalWaitTask *task : tasks) {
			lua_State *T = task->GetThread();
			SbxThreadData *udata = luaSBX_getthreaddata(T);
			if (udata->global->scheduler) {
				udata->global->scheduler->CancelTask(task);
			}
		}
	}
}

uint64_t SignalEmitter::Connect(const std::string &signal, lua_State *L, bool once) {
	uint64_t id = nextId++;
	connections[signal][id] = { L, lua_ref(L, -1), once };
	lua_pop(L, 1);

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	if (udata->signalConnections) {
		udata->signalConnections->AddConnection(this, signal, id);
	}

	return id;
}

bool SignalEmitter::IsConnected(const std::string &signal, uint64_t id) const {
	auto conns = connections.find(signal);
	if (conns == connections.end())
		return false;
	return conns->second.find(id) != conns->second.end();
}

void SignalEmitter::Disconnect(const std::string &signal, uint64_t id, bool cancel, bool updateOwner) {
	auto it = connections[signal].find(id);
	if (it != connections[signal].end()) {
		const Connection &conn = connections[signal][id];

		SbxThreadData *udata = luaSBX_getthreaddata(conn.L);
		if (updateOwner && udata->signalConnections) {
			udata->signalConnections->RemoveConnection(this, id);
		}
		if (cancel && udata->global->scheduler) {
			udata->global->scheduler->CancelEvents(this, id);
		}

		lua_unref(conn.L, conn.ref);
		connections[signal].erase(id);
	}
}

int SignalEmitter::Wait(const std::string &signal, lua_State *L) {
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	if (!udata->global->scheduler)
		luaSBX_noschederror(L);

	SignalWaitTask *task = new SignalWaitTask(L);
	udata->global->scheduler->AddTask(task);
	pendingTasks[signal].push_back(task);

	return lua_yield(L, 0);
}

int SignalEmitter::NumConnections() const {
	int total = 0;
	for (const auto &[emitter, conns] : connections) {
		total += conns.size();
	}
	return total;
}

void SignalConnectionOwner::Clear() {
	for (const auto &[emitter, conns] : connections) {
		for (const auto &[id, connName] : conns) {
			emitter->Disconnect(connName, id, true, false);
		}
	}

	connections.clear();
}

void SignalConnectionOwner::ClearEmitter(SignalEmitter *emitter) {
	connections.erase(emitter);
}

void SignalConnectionOwner::AddConnection(SignalEmitter *emitter, const std::string &name, uint64_t id) {
	connections[emitter][id] = name;
}

void SignalConnectionOwner::RemoveConnection(SignalEmitter *emitter, uint64_t id) {
	connections[emitter].erase(id);
}

bool SignalWaitTask::IsComplete(ResumptionPoint) {
	return pushResults.operator bool();
}

int SignalWaitTask::PushResults() {
	return pushResults(T);
}

} //namespace SBX
