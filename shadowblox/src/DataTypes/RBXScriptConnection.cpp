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

#include "Sbx/DataTypes/RBXScriptConnection.hpp"

#include "lua.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/SignalEmitter.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

RBXScriptConnection::RBXScriptConnection() :
		id(0) {
}

RBXScriptConnection::RBXScriptConnection(std::shared_ptr<SignalEmitter> emitter, std::string name, uint64_t id) :
		emitter(std::move(emitter)), name(std::move(name)), id(id) {
}

void RBXScriptConnection::Register(lua_State *L) {
	using B = LuauClassBinder<RBXScriptConnection>;

	if (!B::IsInitialized()) {
		B::Init("RBXScriptConnection", "RBXScriptConnection", RBXScriptConnectionUdata);

		B::BindPropertyReadOnly<"Connected", &RBXScriptConnection::IsConnected, NoneSecurity>();
		B::BindMethod<"Disconnect", &RBXScriptConnection::Disconnect, NoneSecurity>();

		B::BindToString<&RBXScriptConnection::ToString>();
	}

	B::InitMetatable(L);
}

bool RBXScriptConnection::IsConnected() const {
	return emitter->IsConnected(name, id);
}

void RBXScriptConnection::Disconnect() {
	emitter->Disconnect(name, id);
}

const char *RBXScriptConnection::ToString() const {
	return "Connection";
}

} //namespace SBX::DataTypes

namespace SBX {

using DataTypes::RBXScriptConnection;
UDATA_STACK_OP_IMPL(RBXScriptConnection, "RBXScriptConnection", "RBXScriptConnection", RBXScriptConnectionUdata, DTOR(RBXScriptConnection));

} //namespace SBX
