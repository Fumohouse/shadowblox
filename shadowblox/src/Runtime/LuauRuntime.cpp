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

#include "Sbx/Runtime/LuauRuntime.hpp"

#include <cstdint>
#include <mutex>

#include "Luau/CodeGen.h"
#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"

ThreadHandle::ThreadHandle(lua_State *L) :
		L(L) {
	if (std::mutex *mut = luaSBX_getthreaddata(L)->mutex.get())
		mut->lock();
}

ThreadHandle::~ThreadHandle() {
	if (std::mutex *mut = luaSBX_getthreaddata(L)->mutex.get())
		mut->unlock();
}

ThreadHandle::operator lua_State *() const { return L; }

LuauRuntime::LuauRuntime(void (*initCallback)(lua_State *)) :
		initCallback(initCallback) {
	vms[CoreVM] = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	initVM(vms[CoreVM]);

	vms[UserVM] = luaSBX_newstate(UserVM, GameScriptIdentity);
	initVM(vms[UserVM]);
}

LuauRuntime::~LuauRuntime() {
	for (lua_State *&L : vms) {
		luaSBX_close(L);
		L = nullptr;
	}
}

void LuauRuntime::initVM(lua_State *L) {
	if (initCallback)
		initCallback(L);

	// Seal main global state
	luaL_sandbox(L);

	if (Luau::CodeGen::isSupported())
		Luau::CodeGen::create(L);
}

ThreadHandle LuauRuntime::getVM(VMType type) {
	return vms[type];
}

void LuauRuntime::gcStep(const uint32_t *step, double delta) {
	for (int i = 0; i < VM_MAX; i++) {
		ThreadHandle L = getVM(VMType(i));
		lua_gc(L, LUA_GCSTEP, step[i] * delta);
	}
}

void LuauRuntime::gcSize(int32_t *outBuffer) {
	for (int i = 0; i < VM_MAX; i++) {
		ThreadHandle L = getVM(VMType(i));
		outBuffer[i] = lua_gc(L, LUA_GCCOUNT, 0);
	}
}
