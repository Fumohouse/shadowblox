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

#include <cstdlib>
#include <memory>
#include <mutex>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Stack.hpp"

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
		udata->mutex = parentUdata->mutex;
		udata->userdata = parentUdata->userdata;
	}

	return udata;
}

static void luaSBX_userthread(lua_State *LP, lua_State *L) {
	if (LP) {
		luaSBX_initthreaddata(LP, L);
	} else {
		SbxThreadData *udata = luaSBX_getthreaddata(L);
		if (udata) {
			lua_setthreaddata(L, nullptr);
			delete udata;
		}
	}
}

lua_State *luaSBX_newstate(LuauRuntime::VMType vmType, SbxIdentity defaultIdentity) {
	lua_State *L = lua_newstate(luaSBX_alloc, nullptr);

	// Base libraries
	luaL_openlibs(L);
	LuauStackOp<int64_t>::initMetatable(L);

	SbxThreadData *udata = luaSBX_initthreaddata(nullptr, L);
	udata->vmType = vmType;
	udata->identity = defaultIdentity;
	udata->mutex = std::make_shared<std::mutex>();

	lua_Callbacks *callbacks = lua_callbacks(L);
	callbacks->userthread = luaSBX_userthread;

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
	if (udata) {
		lua_setthreaddata(L, nullptr);
		delete udata;
	}

	lua_close(L);
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
			return PluginSecurity | LocalUserSecurity | WritePlayerSecurity | RobloxScriptSecurity | RobloxSecurity | NotAccessibleSecurity;
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
