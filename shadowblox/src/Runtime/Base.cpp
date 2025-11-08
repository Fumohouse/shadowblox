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

#include "Sbx/Runtime/Base.hpp"

#include <cstdint>
#include <cstdlib>

#include "Luau/CodeGen.h"
#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Logger.hpp"
#include "Sbx/Runtime/SignalEmitter.hpp"
#include "Sbx/Runtime/Stack.hpp"
#include "Sbx/Runtime/TaskScheduler.hpp"

namespace SBX {

// Based on the default implementation seen in the Lua 5.1 reference
static void *luaSBX_alloc(void *, void *ptr, size_t, size_t nsize) {
	if (nsize == 0) {
		free(ptr);
		return nullptr;
	}

	return realloc(ptr, nsize);
}

static SbxThreadData *luaSBX_initthreaddata(lua_State *LP, lua_State *L) {
	SbxThreadData *udata = new SbxThreadData;
	lua_setthreaddata(L, udata);

	if (LP) {
		SbxThreadData *parentUdata = luaSBX_getthreaddata(LP);
		udata->vmType = parentUdata->vmType;
		udata->identity = parentUdata->identity;
		udata->additionalCapability = parentUdata->additionalCapability;
		udata->objRegistry = parentUdata->objRegistry;
		udata->weakObjRegistry = parentUdata->weakObjRegistry;
		udata->signalConnections = parentUdata->signalConnections;
		udata->global = parentUdata->global;
		udata->userdata = parentUdata->userdata;
	}

	return udata;
}

static void luaSBX_userthread(lua_State *LP, lua_State *L) {
	if (LP) {
		luaSBX_initthreaddata(LP, L);
	} else {
		SbxThreadData *udata = luaSBX_getthreaddata(L);

		if (udata->global->scheduler) {
			udata->global->scheduler->CancelThread(L);
		}

		if (udata->signalConnections) {
			udata->signalConnections->Clear();
		}

		lua_setthreaddata(L, nullptr);
		delete udata;
	}
}

lua_State *luaSBX_newstate(VMType vmType, SbxIdentity defaultIdentity) {
	lua_State *L = lua_newstate(luaSBX_alloc, nullptr);

	if (Luau::CodeGen::isSupported())
		Luau::CodeGen::create(L);

	// Base libraries
	luaL_openlibs(L);
	LuauStackOp<int64_t>::InitMetatable(L);
	luaSBX_openlogger(L);
	luaSBX_opensched(L);

	SbxThreadData *udata = luaSBX_initthreaddata(nullptr, L);
	udata->vmType = vmType;
	udata->identity = defaultIdentity;
	udata->global = new SbxGlobalThreadData;

	// Overridden in LuauRuntime to be synced with other VMs
	udata->global->initTimestamp = lua_clock();

	lua_Callbacks *callbacks = lua_callbacks(L);
	callbacks->userthread = luaSBX_userthread;

	// Strong value object registry
	lua_newtable(L);
	udata->objRegistry = lua_ref(L, -1);
	lua_pop(L, 1);

	lua_newtable(L);

	// Weak value object registry
	lua_newtable(L);
	lua_pushstring(L, "v");
	lua_setfield(L, -2, "__mode");
	lua_setreadonly(L, -1, true);
	lua_setmetatable(L, -2);

	udata->weakObjRegistry = lua_ref(L, -1);
	lua_pop(L, 1);

	return L;
}

lua_State *luaSBX_newthread(lua_State *L, SbxIdentity identity) {
	lua_State *T = lua_newthread(L);
	SbxThreadData *udata = luaSBX_getthreaddata(T);
	udata->identity = identity;
	return T;
}

SbxThreadData *luaSBX_getthreaddata(lua_State *L) {
	return reinterpret_cast<SbxThreadData *>(lua_getthreaddata(L));
}

void luaSBX_close(lua_State *L) {
	L = lua_mainthread(L);

	SbxThreadData *udata = luaSBX_getthreaddata(L);

	if (udata->global->scheduler) {
		udata->global->scheduler->CancelThread(L);
	}

	if (udata->signalConnections) {
		udata->signalConnections->Clear();
	}

	lua_close(L);

	delete udata->global;
	delete udata;
}

int32_t identityToCapabilities(SbxIdentity identity) {
	switch (identity) {
		case AnonymousIdentity:
			return NoneSecurity;
		case LocalGuiIdentity:
			return PluginSecurity | LocalUserSecurity;
		case GameScriptIdentity:
			return NoneSecurity;
		case ElevatedGameScriptIdentity:
			return PluginSecurity | LocalUserSecurity | RobloxScriptSecurity | InternalTestSecurity;
		case CommandBarIdentity:
			return PluginSecurity | LocalUserSecurity;
		case StudioPluginIdentity:
			return PluginSecurity;
		case ElevatedStudioPluginIdentity:
			return PluginSecurity | LocalUserSecurity | RobloxScriptSecurity | AssistantSecurity | InternalTestSecurity;
		case COMIdentity:
		case WebServiceIdentity:
			return PluginSecurity | LocalUserSecurity | WritePlayerSecurity | RobloxScriptSecurity | RobloxSecurity | NotAccessibleSecurity;
		case ReplicatorIdentity:
			return WritePlayerSecurity | RobloxScriptSecurity;
		case AssistantIdentity:
			return AssistantSecurity | PluginSecurity | LocalUserSecurity;
		case OpenCloudSessionIdentity:
			return OpenCloudSecurity;
		case TestingGameScriptIdentity:
			return InternalTestSecurity;

		default:
			return NoneSecurity;
	}
}

bool luaSBX_iscapability(lua_State *L, SbxCapability capability) {
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	int32_t threadCapability = identityToCapabilities(udata->identity) | udata->additionalCapability;
	return (threadCapability & capability) == capability;
}

void luaSBX_checkcapability(lua_State *L, SbxCapability capability, const char *actionDesc) {
	if (!luaSBX_iscapability(L, capability)) {
		int permId = 0;
		int32_t curr = capability;
		while (curr >>= 1)
			permId++;

		SbxThreadData *udata = luaSBX_getthreaddata(L);
		luaL_error(
				L, "The current identity (%d) cannot %s (lacking permission %d)",
				udata->identity, actionDesc, permId);
	}
}

bool luaSBX_pushregistry(lua_State *L, void *ptr, void (*push)(lua_State *L, void *ptr), bool weak) {
	SbxThreadData *udata = luaSBX_getthreaddata(L);
	lua_getref(L, weak ? udata->weakObjRegistry : udata->objRegistry);
	lua_pushlightuserdata(L, ptr);
	lua_gettable(L, -2);

	bool res = lua_isnil(L, -1);

	if (res) {
		lua_pop(L, 1); // nil

		push(L, ptr);
		lua_pushlightuserdata(L, ptr);
		lua_pushvalue(L, -2);
		lua_settable(L, -4);
	}

	lua_remove(L, -2); // table
	return res;
}

void luaSBX_debugcallbacks(lua_State *L) {
	lua_Callbacks *cb = lua_callbacks(L);
	cb->interrupt = luaSBX_cbinterrupt;
}

void luaSBX_cbinterrupt(lua_State *L, int gc) {
	SbxThreadData *udata = luaSBX_getthreaddata(L);

	if (udata->interruptDeadline == 0)
		return;

	if (gc < 0 && (uint64_t)(lua_clock() * 1e6) > udata->interruptDeadline) {
		lua_checkstack(L, 1);
		luaL_error(L, "Script timed out: exhausted allowed execution time");
	}
}

int luaSBX_resume(lua_State *L, lua_State *from, int nargs, double timeout) {
	// TODO: Possible to avoid setting timeouts?
	// Maybe use preprocessor defines
	luaSBX_getthreaddata(L)->interruptDeadline = (uint64_t)((lua_clock() + timeout) * 1e6);

	int status = lua_resume(L, from, nargs);

	// TODO: Debug break if applicable
	if (status != LUA_OK && status != LUA_YIELD) {
		SbxThreadData *udata = luaSBX_getthreaddata(L);
		if (udata->global->logger) {
			udata->global->logger->Error(lua_tostring(L, -1));
		}
	}

	return status;
}

int luaSBX_pcall(lua_State *L, int nargs, int nresults, int errfunc, double timeout) {
	// TODO: Possible to avoid setting timeouts?
	// Maybe use preprocessor defines
	luaSBX_getthreaddata(L)->interruptDeadline = (uint64_t)((lua_clock() + timeout) * 1e6);

	int status = lua_pcall(L, nargs, nresults, errfunc);

	// TODO: Debug break if applicable
	if (status != LUA_OK && status != LUA_YIELD) {
		SbxThreadData *udata = luaSBX_getthreaddata(L);
		if (udata->global->logger) {
			udata->global->logger->Error(lua_tostring(L, -1));
		}
	}

	return status;
}

} //namespace SBX
