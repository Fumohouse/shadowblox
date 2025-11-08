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

#include "lua.h"

#define luaSBX_noproperror(L, propertyName, className) luaL_error(L, "%s is not a valid member of %s", propertyName, className)
#define luaSBX_propwriteonlyerror(L, propertyName, className) luaL_error(L, "%s member of %s is write-only and cannot be cannot be read", propertyName, className)
#define luaSBX_propreadonlyerror(L, propertyName, className) luaL_error(L, "%s member of %s is read-only and cannot be assigned to", propertyName, className)

#define luaSBX_aritherror1type(L, op, type) luaL_error(L, "attempt to perform arithmetic (%s) on %s", op, type)
#define luaSBX_aritherror2type(L, op, lhsType, rhsType) luaL_error(L, "attempt to perform arithmetic (%s) on %s and %s", op, lhsType, rhsType)

#define luaSBX_nonamecallatomerror(L) luaL_error(L, "no namecallatom")
#define luaSBX_nomethoderror(L, methodName, className) luaSBX_noproperror(L, methodName, className)

#define luaSBX_missingselferror(L, func) luaL_error(L, "Expected ':' not '.' calling member function %s", func)
#define luaSBX_missingargerror(L, num) luaL_error(L, "Argument %d missing or nil", num)
#define luaSBX_casterror(L, from, to) luaL_error(L, "Unable to cast %s to %s", from, to)

#define luaSBX_noschederror(L) luaL_error(L, "missing task scheduler")
#define luaSBX_nologerror(L) luaL_error(L, "missing logger")

namespace SBX {

class Logger;
class TaskScheduler;
class SignalConnectionOwner;

enum UdataTag : uint8_t {
	Int64Udata = 0,

	EnumItemUdata,
	EnumUdata,
	EnumsUdata,
	RBXScriptSignalUdata,
	RBXScriptConnectionUdata,

	Test1Udata = 124,
	Test2Udata = 125,
	Test3Udata = 126,
	Test4Udata = 127,
};

// https://github.com/Pseudoreality/Roblox-Identities
enum SbxIdentity : uint8_t {
	AnonymousIdentity = 0,
	LocalGuiIdentity,
	GameScriptIdentity,
	ElevatedGameScriptIdentity,
	CommandBarIdentity,
	StudioPluginIdentity,
	ElevatedStudioPluginIdentity,
	COMIdentity,
	WebServiceIdentity,
	ReplicatorIdentity,
	AssistantIdentity,
	OpenCloudSessionIdentity,
	TestingGameScriptIdentity,

	IdentityMax
};

enum SbxCapability {
	// NoneSecurity is ID 0
	// All others are 1 << ID
	NoneSecurity = 0,
	PluginSecurity = 1 << 1,
	LocalUserSecurity = 1 << 3,
	WritePlayerSecurity = 1 << 4,
	RobloxScriptSecurity = 1 << 5,
	RobloxSecurity = 1 << 6,
	NotAccessibleSecurity = 1 << 7,

	AssistantSecurity = 1 << 16,
	InternalTestSecurity = 1 << 17,
	OpenCloudSecurity = 1 << 18,
	RemoteCommandSecurity = 1 << 19,
	UnknownSecurity = 1 << 20
};

enum VMType : uint8_t {
	CoreVM = 0,
	UserVM,

	VMMax
};

struct SbxGlobalThreadData {
	double initTimestamp = 0.0;
	Logger *logger = nullptr;
	TaskScheduler *scheduler = nullptr;
};

struct SbxThreadData {
	VMType vmType = VMMax;
	SbxIdentity identity = AnonymousIdentity;
	int32_t additionalCapability = 0;
	uint64_t interruptDeadline = 0;

	int objRegistry = LUA_NOREF;
	int weakObjRegistry = LUA_NOREF;

	SignalConnectionOwner *signalConnections = nullptr;

	SbxGlobalThreadData *global = nullptr;
	void *userdata = nullptr;
};

lua_State *luaSBX_newstate(VMType vmType, SbxIdentity defaultIdentity);
lua_State *luaSBX_newthread(lua_State *L, SbxIdentity identity);
SbxThreadData *luaSBX_getthreaddata(lua_State *L);
void luaSBX_close(lua_State *L);

int32_t identityToCapabilities(SbxIdentity identity);

bool luaSBX_iscapability(lua_State *L, SbxCapability capability);
void luaSBX_checkcapability(lua_State *L, SbxCapability capability, const char *actionDesc);

bool luaSBX_pushregistry(lua_State *L, void *ptr, void (*push)(lua_State *L, void *ptr), bool weak);

void luaSBX_debugcallbacks(lua_State *L);
void luaSBX_cbinterrupt(lua_State *L, int gc);

int luaSBX_resume(lua_State *L, lua_State *from, int nargs, double timeout = 10.0);
int luaSBX_pcall(lua_State *L, int nargs, int nresults, int errfunc, double timeout = 10.0);

} //namespace SBX
