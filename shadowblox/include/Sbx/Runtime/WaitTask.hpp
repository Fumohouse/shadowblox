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

#include "Sbx/Runtime/TaskScheduler.hpp"

namespace SBX {

class WaitTask : public ScheduledTask {
public:
	WaitTask(lua_State *T, double duration, bool legacyThrottling);

	bool CanThrottle() override { return legacyThrottling; }
	bool IsComplete(ResumptionPoint) override;
	int PushResults() override;
	void Update(uint64_t frame, double delta) override;

private:
	double elapsed;
	double duration;
	uint64_t lastFrame;
	bool legacyThrottling;
};

int luaSBX_wait(lua_State *L);
int luaSBX_taskwait(lua_State *L);

} //namespace SBX
