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

#include "Sbx/DataTypes/RBXScriptSignal.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "ltm.h"
#include "lua.h"
#include "lualib.h"

#include "Sbx/DataTypes/RBXScriptConnection.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/SignalEmitter.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

RBXScriptSignal::RBXScriptSignal() {
}

RBXScriptSignal::RBXScriptSignal(std::shared_ptr<SignalEmitter> emitter, std::string name) :
		emitter(std::move(emitter)), name(std::move(name)) {
}

void RBXScriptSignal::Register(lua_State *L) {
	using B = LuauClassBinder<RBXScriptSignal>;

	if (!B::IsInitialized()) {
		B::Init("RBXScriptSignal", "RBXScriptSignal", RBXScriptSignalUdata);

		B::BindLuauMethod<"Connect", RBXScriptSignal::Connect>();
		B::BindLuauMethod<"Once", RBXScriptSignal::Once>();
		B::BindLuauMethod<"Wait", RBXScriptSignal::Wait>();

		B::BindToString<&RBXScriptSignal::ToString>();
		B::BindBinaryOp<TM_EQ, &RBXScriptSignal::operator== >(LuauStackOp<RBXScriptSignal>::Is, LuauStackOp<RBXScriptSignal>::Is);
	}

	B::InitMetatable(L);
}

int RBXScriptSignal::Connect(lua_State *L) {
	RBXScriptSignal *self = LuauStackOp<RBXScriptSignal *>::Check(L, 1);
	if (!lua_isfunction(L, 2)) {
		luaL_error(L, "Attempt to connect failed: Passed value is not a function");
	}

	uint64_t id = self->emitter->Connect(self->name, L, false);
	LuauStackOp<RBXScriptConnection>::Push(L, RBXScriptConnection(self->emitter, self->name, id));
	return 1;
}

int RBXScriptSignal::Once(lua_State *L) {
	RBXScriptSignal *self = LuauStackOp<RBXScriptSignal *>::Check(L, 1);
	if (!lua_isfunction(L, 2)) {
		luaL_error(L, "Attempt to connect failed: Passed value is not a function");
	}

	uint64_t id = self->emitter->Connect(self->name, L, true);
	LuauStackOp<RBXScriptConnection>::Push(L, RBXScriptConnection(self->emitter, self->name, id));
	return 1;
}

int RBXScriptSignal::Wait(lua_State *L) {
	RBXScriptSignal *self = LuauStackOp<RBXScriptSignal *>::Check(L, 1);
	return self->emitter->Wait(self->name, L);
}

std::string RBXScriptSignal::ToString() const {
	return "Signal " + name;
}

bool RBXScriptSignal::operator==(const RBXScriptSignal &other) const {
	return emitter == other.emitter && name == other.name;
}

} //namespace SBX::DataTypes

namespace SBX {

using DataTypes::RBXScriptSignal;
UDATA_STACK_OP_IMPL(RBXScriptSignal, "RBXScriptSignal", "RBXScriptSignal", RBXScriptSignalUdata, DTOR(RBXScriptSignal));

} //namespace SBX
