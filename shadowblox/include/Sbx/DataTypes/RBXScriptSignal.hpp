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

#include <memory>
#include <string>

#include "lua.h"

#include "Sbx/Runtime/Stack.hpp"

namespace SBX {

class SignalEmitter;

}

namespace SBX::DataTypes {

/**
 * @brief This class implements Roblox's [`RBXScriptSignal`](https://create.roblox.com/docs/en-us/reference/engine/datatypes/RBXScriptSignal)
 * data type.
 */
class RBXScriptSignal {
public:
	RBXScriptSignal(); // required for stack operation
	RBXScriptSignal(std::shared_ptr<SignalEmitter> emitter, std::string name);

	static void Register(lua_State *L);

	static int Connect(lua_State *L);
	static int Once(lua_State *L);
	static int Wait(lua_State *L);

	std::string ToString() const;
	bool operator==(const RBXScriptSignal &other) const;

private:
	// Hold this reference to allow this object to continue working after the
	// emitter owner is collected.
	std::shared_ptr<SignalEmitter> emitter;
	std::string name;
};

} //namespace SBX::DataTypes

namespace SBX {

STACK_OP_UDATA_DEF(DataTypes::RBXScriptSignal);

}
