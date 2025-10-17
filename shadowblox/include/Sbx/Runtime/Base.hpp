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
#include <memory>
#include <mutex>

#include "Sbx/Runtime/LuauRuntime.hpp"

enum UdataTag {
	Int64Udata = 0,

	Test1Udata = 124,
	Test2Udata = 125,
	Test3Udata = 126,
	Test4Udata = 127,
};

// https://github.com/Pseudoreality/Roblox-Identities
enum SbxIdentity {
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

	IDENTITY_MAX
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

struct SbxThreadData {
	LuauRuntime::VMType vmType = LuauRuntime::VM_MAX;
	SbxIdentity identity = AnonymousIdentity;
	int32_t additionalCapability = 0;
	std::shared_ptr<std::mutex> mutex;
	uint64_t interruptDeadline = 0;

	void *userdata = nullptr;
};

lua_State *luaSBX_newstate(LuauRuntime::VMType vmType, SbxIdentity defaultIdentity);
lua_State *luaSBX_newthread(lua_State *L, SbxIdentity identity);
SbxThreadData *luaSBX_getthreaddata(lua_State *L);
void luaSBX_close(lua_State *L);

int32_t identityToCapabilities(SbxIdentity identity);

bool luaSBX_iscapability(lua_State *L, SbxCapability capability);
void luaSBX_checkcapability(lua_State *L, SbxCapability capability, const char *actionDesc);
