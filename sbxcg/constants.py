# SPDX-License-Identifier: LGPL-3.0-or-later
################################################################################
# shadowblox - https://git.seki.pw/Fumohouse/shadowblox
#
# Copyright 2025-present ksk.
# Copyright 2025-present shadowblox contributors.
#
# Licensed under the GNU Lesser General Public License version 3.0 or later.
# See COPYRIGHT.txt for more details.
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
################################################################################

header_regex_cpp = r"// SPDX-License-Identifier: .*\n/\*[\s\S]*?\*/\n*"
header_cpp = """// SPDX-License-Identifier: LGPL-3.0-or-later
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

"""

header_regex_luau = r"-- SPDX-License-Identifier: .*\n--\[=*\[[\s\S]*?\]=*\]\n*"
header_luau = """-- SPDX-License-Identifier: LGPL-3.0-or-later
--[============================================================================[
-- shadowblox - https://git.seki.pw/Fumohouse/shadowblox
--
-- Copyright 2025-present ksk.
-- Copyright 2025-present shadowblox contributors.
--
-- Licensed under the GNU Lesser General Public License version 3.0 or later.
-- See COPYRIGHT.txt for more details.
--
-- This program is free software: you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by the Free
-- Software Foundation, either version 3 of the License, or (at your option)
-- any later version.
--
-- This program is distributed in the hope that it will be useful, but WITHOUT
-- ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
-- FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program. If not, see <https://www.gnu.org/licenses/>.
--]============================================================================]

"""

header_regex_py = r"# SPDX-License-Identifier: .*\n#{2,}\n[\s\S]*?#{2,}\n*"
header_py = """# SPDX-License-Identifier: LGPL-3.0-or-later
################################################################################
# shadowblox - https://git.seki.pw/Fumohouse/shadowblox
#
# Copyright 2025-present ksk.
# Copyright 2025-present shadowblox contributors.
#
# Licensed under the GNU Lesser General Public License version 3.0 or later.
# See COPYRIGHT.txt for more details.
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
################################################################################

"""

type_map = {
    # Primitives
    "null": {"type": "void"},
    "bool": {"type": "bool"},
    "int": {"type": "int"},
    "int64": {"type": "int64_t", "include": "<cstdint>"},
    "float": {"type": "float"},
    "double": {"type": "double"},
    "string": {"type": "const char *"},
    # Luau
    "Function": {"type": "Variant", "include": '"Sbx/Classes/Variant.hpp"'},
    "Dictionary": {"type": "Variant", "include": '"Sbx/Classes/Variant.hpp"'},
    "Array": {"type": "Variant", "include": '"Sbx/Classes/Variant.hpp"'},
    # Data types
    "RBXScriptSignal": {
        "type": "DataTypes::RBXScriptSignal",
        "include": '"Sbx/DataTypes/RBXScriptSignal.hpp"',
    },
    # Other
    "Variant": {"type": "Variant", "include": '"Sbx/Classes/Variant.hpp"'},
}

auto_tags = {"ReadOnly", "CustomLuaState", "NotScriptable"}
