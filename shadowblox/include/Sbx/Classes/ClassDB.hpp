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

#include <any>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Classes/Variant.hpp"
#include "Sbx/DataTypes/Enum.hpp"
#include "Sbx/DataTypes/EnumTypes.gen.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/Binder.hpp"
#include "Sbx/Runtime/ClassBinder.hpp"
#include "Sbx/Runtime/Stack.hpp"
#include "Sbx/Runtime/StringMap.hpp"

namespace SBX::Classes {

class Object;

enum class MemoryCategory : uint8_t {
	Instances,
	Script,
	Gui,
	Internal,
	GraphicsTexture,
	Animation,
};

enum class ClassTag : uint8_t {
	Deprecated,
	NotCreatable,
	NotBrowsable,
	NotReplicated,
	Service,
	PlayerReplicated,
};

enum class MemberTag : uint8_t {
	Deprecated,
	NotReplicated,
	Hidden,
	NotBrowsable,
	Yields,
	NoYield,
	CanYield,

	// Automatic: Do not need to specify
	ReadOnly,
	CustomLuaState,
	NotScriptable,
};

enum class ThreadSafety : uint8_t {
	Unsafe,
	ReadSafe,
	Safe,
};

template <typename T>
std::string getTypeName() {
	if constexpr (std::is_same_v<T, void>) {
		return "null";
	} else if constexpr (DataTypes::EnumClassToEnum<T>::enumType) {
		constexpr DataTypes::Enum *E = DataTypes::EnumClassToEnum<T>::enumType;
		return E->GetName();
	} else {
		return LuauStackOp<T>::NAME;
	}
}

// See Binder.hpp
template <typename>
struct IsTuple : std::false_type {};

template <typename... T>
struct IsTuple<std::tuple<T...>> : std::true_type {
	static std::vector<std::string> GetTypeNames() {
		return { getTypeName<T>()... };
	}
};

template <typename T>
std::vector<std::string> getTypeNames() {
	if constexpr (IsTuple<T>::value) {
		return IsTuple<T>::GetTypeNames();
	} else {
		return { getTypeName<T>() };
	}
}

class ClassDB {
public:
	struct Parameter {
		std::string name;
		std::string type;
	};

	struct Function {
		std::string name;
		std::vector<std::string> returnType;
		std::vector<Parameter> parameters;
		std::unordered_set<MemberTag> tags;
		SbxCapability security = NoneSecurity;
		ThreadSafety safety = ThreadSafety::Unsafe;
	};

	struct Property {
		std::string name;
		std::string changedSignal;
		std::string category;
		std::any (*getter)(void *self) = nullptr;
		bool (*setter)(void *self, const std::any &value) = nullptr;
		std::string type;
		std::unordered_set<MemberTag> tags;
		SbxCapability readSecurity = NoneSecurity;
		SbxCapability writeSecurity = NoneSecurity;
		ThreadSafety safety = ThreadSafety::Unsafe;
		bool canLoad = false;
		bool canSave = false;
	};

	struct Signal {
		std::string name;
		std::vector<Parameter> parameters;
		std::unordered_set<MemberTag> tags;
		SbxCapability security = NoneSecurity;
		bool unlisted = false; // for *Changed, etc.
	};

	struct Callback {
		std::string name;
		void (*func)(void *self, lua_State *L);
		std::vector<std::string> returnType;
		std::vector<Parameter> parameters;
		std::unordered_set<MemberTag> tags;
		SbxCapability security = NoneSecurity;
		ThreadSafety safety = ThreadSafety::Unsafe;
	};

	struct ClassInfo {
		std::string name;
		std::string parent;
		MemoryCategory category;
		std::unordered_set<ClassTag> tags;

		StringMap<Function> functions;
		StringMap<Property> properties;
		StringMap<Signal> signals;
		StringMap<Callback> callbacks;
	};

	template <typename Sig>
	struct FuncUtils;

	template <typename R, typename... Args>
	struct FuncUtils<R(Args...)> {
		using RetType = R;

		template <typename... VarArgs>
		static std::vector<Parameter> GetParams(VarArgs... paramNames) {
			return { { paramNames, getTypeName<Args>() }... };
		}
	};

	template <typename R, typename... Args>
	struct FuncUtils<R (*)(Args...)> {
		using RetType = R;

		template <typename... VarArgs>
		static std::vector<Parameter> GetParams(VarArgs... paramNames) {
			return { { paramNames, getTypeName<Args>() }... };
		}
	};

	template <typename T, typename R, typename... Args>
	struct FuncUtils<R (T::*)(Args...)> {
		using RetType = R;

		template <typename... VarArgs>
		static std::vector<Parameter> GetParams(VarArgs... paramNames) {
			return { { paramNames, getTypeName<Args>() }... };
		}
	};

	template <typename T, typename R, typename... Args>
	struct FuncUtils<R (T::*)(Args...) const> {
		using RetType = R;

		template <typename... VarArgs>
		static std::vector<Parameter> GetParams(VarArgs... paramNames) {
			return { { paramNames, getTypeName<Args>() }... };
		}
	};

	template <typename T, MemoryCategory category, ClassTag... tags>
	static void AddClass(const char *parent) {
		ClassInfo info;
		info.name = T::NAME;
		info.parent = parent;
		info.category = category;
		info.tags = { tags... };

		classes[T::NAME] = std::move(info);
		if constexpr (((tags != ClassTag::NotCreatable) && ...)) {
			constructors[T::NAME] = []() -> std::shared_ptr<Object> {
				return std::make_shared<T>();
			};
		}
		registerCallbacks.push_back(LuauClassBinder<T>::InitMetatable);
	}

	template <typename T, StringLiteral name, auto F, SbxCapability capability, ThreadSafety safety, typename... Args>
	static void BindMethod(std::unordered_set<MemberTag> tags, Args... paramNames) {
		classes[T::NAME].functions[name.value] = CreateFunction<decltype(F)>(
				name.value, capability, safety, std::move(tags), paramNames...);
		LuauClassBinder<T>::template BindMethod<name, F, capability>();
	}

	template <typename T, StringLiteral name, typename Sig, lua_CFunction F, SbxCapability capability, ThreadSafety safety, typename... Args>
	static void BindLuauMethod(std::unordered_set<MemberTag> tags, Args... paramNames) {
		tags.insert(MemberTag::CustomLuaState);
		classes[T::NAME].functions[name.value] = CreateFunction<Sig>(
				name.value, capability, safety, std::move(tags), paramNames...);
		LuauClassBinder<T>::template BindLuauMethod<name, F>();
	}

	template <typename T, StringLiteral name, StringLiteral category, auto G, SbxCapability getCapability, auto S, SbxCapability setCapability, ThreadSafety safety, bool canLoad, bool canSave>
	static void BindProperty(std::unordered_set<MemberTag> tags) {
		classes[T::NAME].properties[name.value] = CreateProperty<T, G, S>(
				name.value, category.value, getCapability, setCapability, safety, canLoad, canSave, std::move(tags));
		classes[T::NAME].signals[std::string(name.value) + "Changed"] = CreateSignal<void()>(
				std::string(name.value) + "Changed", getCapability, {}, true);
		LuauClassBinder<T>::template BindProperty<name, G, getCapability, S, setCapability>();
	}

	template <typename T, StringLiteral name, StringLiteral category, auto G, SbxCapability getCapability, ThreadSafety safety, bool canSave>
	static void BindPropertyReadOnly(std::unordered_set<MemberTag> tags) {
		tags.insert(MemberTag::ReadOnly);
		classes[T::NAME].properties[name.value] = CreateProperty<T, G, nullptr>(
				name.value, category.value, getCapability, NoneSecurity, safety, false, canSave, std::move(tags));
		classes[T::NAME].signals[std::string(name.value) + "Changed"] = CreateSignal<void()>(
				std::string(name.value) + "Changed", getCapability, {}, true);
		LuauClassBinder<T>::template BindPropertyReadOnly<name, G, getCapability>();
	}

	template <typename T, StringLiteral name, StringLiteral category, auto G, auto S, ThreadSafety safety, bool canLoad, bool canSave>
	static void BindPropertyNotScriptable(std::unordered_set<MemberTag> tags) {
		tags.insert(MemberTag::NotScriptable);
		classes[T::NAME].properties[name.value] = CreateProperty<T, G, S>(
				name.value, category.value, NoneSecurity, NoneSecurity, safety, canLoad, canSave, std::move(tags));
	}

	template <typename T, StringLiteral name, StringLiteral category, auto G, ThreadSafety safety, bool canSave>
	static void BindPropertyNotScriptableReadOnly(std::unordered_set<MemberTag> tags) {
		tags.insert(MemberTag::NotScriptable);
		tags.insert(MemberTag::ReadOnly);
		classes[T::NAME].properties[name.value] = CreateProperty<T, G, nullptr>(
				name.value, category.value, NoneSecurity, NoneSecurity, safety, false, canSave, std::move(tags));
	}

	template <typename T, StringLiteral name, typename Sig, SbxCapability capability, typename... Args>
	static void BindSignal(std::unordered_set<MemberTag> tags, Args... paramNames) {
		classes[T::NAME].signals[name.value] = CreateSignal<Sig>(
				name.value, capability, std::move(tags), false, paramNames...);
	}

	template <typename T, StringLiteral name, typename Sig, void (T::*F)(lua_State *), SbxCapability capability, ThreadSafety safety, typename... Args>
	static void BindCallback(std::unordered_set<MemberTag> tags, Args... paramNames) {
		Callback cb;
		cb.name = name.value;
		cb.func = [](void *self, lua_State *L) {
			(((T *)self)->*F)(L);
		};
		cb.returnType = getTypeNames<typename FuncUtils<Sig>::RetType>();
		cb.parameters = FuncUtils<Sig>::GetParams(paramNames...);
		cb.tags = std::move(tags);
		cb.security = capability;
		cb.safety = safety;

		classes[T::NAME].callbacks[name.value] = std::move(cb);
	}

	static const ClassInfo *GetClass(std::string_view className);
	static const Function *GetFunction(std::string_view className, std::string_view funcName);
	static const Property *GetProperty(std::string_view className, std::string_view propName);
	static const Signal *GetSignal(std::string_view className, std::string_view sigName);
	static const Callback *GetCallback(std::string_view className, std::string_view cbName);

	static std::shared_ptr<Object> New(std::string_view className);
	static bool IsA(std::string_view derived, std::string_view base);

	static void Register(lua_State *L);

private:
	template <typename Sig, typename... Args>
	static Function CreateFunction(const char *name, SbxCapability capability, ThreadSafety safety, std::unordered_set<MemberTag> tags, Args... paramNames) {
		Function func;
		func.name = name;
		func.returnType = getTypeNames<typename FuncUtils<Sig>::RetType>();
		func.parameters = FuncUtils<Sig>::GetParams(paramNames...);
		func.tags = std::move(tags);
		func.security = capability;
		func.safety = safety;

		return func;
	}

	template <typename T, auto G, auto S>
	static Property CreateProperty(const char *name, const char *category, SbxCapability getCapability, SbxCapability setCapability, ThreadSafety safety, bool canLoad, bool canSave, std::unordered_set<MemberTag> tags) {
		using PropType = FuncUtils<decltype(G)>::RetType;
		using BaseType = std::decay_t<PropType>;

		Property prop;
		prop.name = name;
		if (!tags.contains(MemberTag::NotScriptable)) {
			prop.changedSignal = prop.name + "Changed";
		}
		prop.category = category;
		prop.getter = [](void *self) -> std::any {
			return (((T *)self)->*G)();
		};
		if constexpr (!std::is_same_v<decltype(S), std::nullptr_t>) {
			prop.setter = [](void *self, const std::any &value) -> bool {
				if (value.type() == typeid(BaseType)) {
					(((T *)self)->*S)(std::any_cast<BaseType>(value));
					return true;
				}

				if constexpr (std::is_enum_v<BaseType>) {
					if (value.type() == typeid(uint32_t)) {
						(((T *)self)->*S)(static_cast<BaseType>(std::any_cast<uint32_t>(value)));
						return true;
					}
				}

				return false;
			};
		}
		prop.type = getTypeName<PropType>();
		prop.tags = std::move(tags);
		prop.readSecurity = getCapability;
		prop.writeSecurity = setCapability;
		prop.safety = safety;
		prop.canLoad = canLoad;
		prop.canSave = canSave;

		return prop;
	}

	template <typename Sig, typename... Args>
	static Signal CreateSignal(std::string_view name, SbxCapability capability, std::unordered_set<MemberTag> tags, bool unlisted, Args... paramNames) {
		Signal signal;
		signal.name = name;
		signal.parameters = FuncUtils<Sig>::GetParams(paramNames...);
		signal.tags = std::move(tags);
		signal.security = capability;
		signal.unlisted = unlisted;

		return signal;
	}

	static StringMap<ClassInfo> classes;
	static StringMap<std::shared_ptr<Object> (*)()> constructors;
	static std::vector<void (*)(lua_State *)> registerCallbacks;
};

#define SBXCLASS(className, parent, category, ...)                                                                    \
public:                                                                                                               \
	className(const className &other) = delete;                                                                       \
	className(className &&other) = delete;                                                                            \
	className &operator=(const className &other) = delete;                                                            \
	className &operator=(className &&other) = delete;                                                                 \
                                                                                                                      \
	static constexpr const char *NAME = #className;                                                                   \
                                                                                                                      \
	const char *GetClassName() const override { return NAME; }                                                        \
                                                                                                                      \
	static void InitializeClass() {                                                                                   \
		static bool initialized = false;                                                                              \
		if (!initialized) {                                                                                           \
			::SBX::Classes::ClassDB::AddClass<className, category, ##__VA_ARGS__>(#parent);                           \
			LuauClassBinder<className>::Init(#className, #className, -1, ::SBX::Classes::Variant::Object);            \
			LuauClassBinder<className>::AddIndexOverride(IndexOverride);                                              \
			LuauClassBinder<className>::AddNewindexOverride(NewindexOverride);                                        \
			BindMembers<className>();                                                                                 \
			initialized = true;                                                                                       \
		}                                                                                                             \
	}                                                                                                                 \
                                                                                                                      \
private:                                                                                                              \
	static int IndexOverride(lua_State *L, const char *propName) {                                                    \
		className *self = ::SBX::LuauStackOp<className *>::Check(L, 1);                                               \
                                                                                                                      \
		if (const ClassDB::Signal *sig = ClassDB::GetSignal(className::NAME, propName)) {                             \
			self->PushSignal(L, sig->name, sig->security);                                                            \
			return 1;                                                                                                 \
		}                                                                                                             \
                                                                                                                      \
		if (ClassDB::GetCallback(className::NAME, propName)) {                                                        \
			luaL_error(L, "%s is a callback member of %s; you can only set the callback value, get is not available", \
					propName, className::NAME);                                                                       \
		}                                                                                                             \
                                                                                                                      \
		return 0;                                                                                                     \
	}                                                                                                                 \
                                                                                                                      \
	static bool NewindexOverride(lua_State *L, const char *propName) {                                                \
		className *self = ::SBX::LuauStackOp<className *>::Check(L, 1);                                               \
                                                                                                                      \
		if (const ClassDB::Callback *cb = ClassDB::GetCallback(className::NAME, propName)) {                          \
			/* Roblox may not perform type checking (e.g., for OnInvoke) */                                           \
			lua_remove(L, 2); /* name */                                                                              \
			lua_remove(L, 1); /* self */                                                                              \
			cb->func(self, L);                                                                                        \
			return true;                                                                                              \
		}                                                                                                             \
                                                                                                                      \
		return false;                                                                                                 \
	}

} //namespace SBX::Classes
