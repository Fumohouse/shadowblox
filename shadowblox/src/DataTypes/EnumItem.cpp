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

#include "Sbx/DataTypes/EnumItem.hpp"

#include <string>

#include "lua.h"

#include "Sbx/Classes/Variant.hpp"
#include "Sbx/DataTypes/Enum.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/Stack.hpp"

namespace SBX::DataTypes {

EnumItem::EnumItem(const char *name, int value, Enum *enumType) :
		name(name), value(value), enumType(enumType) {
	enumType->AddItem(this);
}

void EnumItem::Register(lua_State *L) {
	using B = LuauClassBinder<EnumItem>;

	if (!B::IsInitialized()) {
		B::Init("EnumItem", "EnumItem", EnumItemUdata, Classes::Variant::EnumItem);
		B::BindToString<&EnumItem::ToString>();
		B::BindPropertyReadOnly<"Name", &EnumItem::GetName, NoneSecurity>();
		B::BindPropertyReadOnly<"Value", &EnumItem::GetValue, NoneSecurity>();
		B::BindPropertyReadOnly<"EnumType", &EnumItem::GetEnumType, NoneSecurity>();
	}

	B::InitMetatable(L);
}

std::string EnumItem::ToString() const {
	return std::string("Enum.") + enumType->name + "." + name;
}

} //namespace SBX::DataTypes

namespace SBX {

REGISTRY_PTR_STACK_OP_IMPL(DataTypes::EnumItem, "EnumItem", "EnumItem", EnumItemUdata, false);

}
