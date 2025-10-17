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

struct lua_State;
class ThreadHandle;

class LuauRuntime {
public:
	enum VMType : uint8_t {
		CoreVM = 0,
		UserVM,

		VMMax
	};

	LuauRuntime(void (*initCallback)(lua_State *));
	~LuauRuntime();

	LuauRuntime(const LuauRuntime &other) = delete;
	LuauRuntime(LuauRuntime &&other) = delete;
	LuauRuntime &operator=(const LuauRuntime &other) = delete;
	LuauRuntime &operator=(LuauRuntime &&other) = delete;

	ThreadHandle getVM(VMType type);

	void gcStep(const uint32_t *step, double delta);
	void gcSize(int32_t *outBuffer);

private:
	void (*initCallback)(lua_State *);
	lua_State *vms[VMMax];
	void initVM(lua_State *L);
};

class ThreadHandle {
public:
	ThreadHandle(lua_State *L);
	~ThreadHandle();

	ThreadHandle(const ThreadHandle &other) = delete;
	ThreadHandle(ThreadHandle &&other) = delete;
	ThreadHandle &operator=(const ThreadHandle &other) = delete;
	ThreadHandle &operator=(ThreadHandle &&other) = delete;

	operator lua_State *() const;

private:
	lua_State *L;
};
