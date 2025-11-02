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

#include "Utils.hpp"

#include <string>

#include "Luau/Compiler.h"
#include "lua.h"

#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Stack.hpp"

using namespace SBX;

ExecOutput luaGD_exec(lua_State *L, const char *src) {
	Luau::CompileOptions opts;
	std::string bytecode = Luau::compile(src, opts);

	ExecOutput output;

	if (luau_load(L, "=exec", bytecode.data(), bytecode.size(), 0) == 0) {
		int status = luaSBX_resume(L, nullptr, 0, 1.0);
		output.status = status;

		if (status == LUA_OK) {
			return output;
		} else if (status == LUA_YIELD) {
			INFO("thread yielded");
			return output;
		}

		std::string error = LuauStackOp<std::string>::Get(L, -1);
		lua_pop(L, 1);

		output.error = error;
		return output;
	}

	output.error = LuauStackOp<std::string>::Get(L, -1);
	lua_pop(L, 1);

	return output;
}
