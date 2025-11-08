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
#include <string>

#include "lua.h"

#include "Sbx/Runtime/Stack.hpp"

namespace SBX {

class SignalEmitter;

}

namespace SBX::DataTypes {

/**
 * @brief This class implements Roblox's [`RBXScriptConnection`](https://create.roblox.com/docs/en-us/reference/engine/datatypes/RBXScriptConnection)
 * data type.
 */
class RBXScriptConnection {
public:
	RBXScriptConnection(); // required for stack operation
	RBXScriptConnection(std::shared_ptr<SignalEmitter> emitter, std::string name, uint64_t id);

	static void Register(lua_State *L);

	bool IsConnected() const;
	void Disconnect();

	const char *ToString() const;

private:
	// Hold this reference to force connections to stay connected once the
	// emitter owner is collected.
	std::shared_ptr<SignalEmitter> emitter;
	std::string name;
	uint64_t id;
};

} //namespace SBX::DataTypes

namespace SBX {

STACK_OP_UDATA_DEF(DataTypes::RBXScriptConnection);

}
