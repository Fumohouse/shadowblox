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

#include "doctest.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "lua.h"
#include "lualib.h"

#include "Sbx/Classes/ClassDB.hpp"
#include "Sbx/Classes/Object.hpp"
#include "Sbx/DataTypes/EnumTypes.gen.hpp"
#include "Sbx/DataTypes/Types.hpp"
#include "Sbx/Runtime/Base.hpp"
#include "Sbx/Runtime/SignalEmitter.hpp"
#include "Utils.hpp"

using namespace SBX;
using namespace SBX::Classes;

TEST_SUITE_BEGIN("Classes/Object");

namespace SBX::Classes {

class TestDerived : public Object {
	SBXCLASS(TestDerived, Object, MemoryCategory::Internal, ClassTag::NotReplicated);

public:
	TestDerived() {
	}

	~TestDerived() override {
		ClearCallback();
	}

	void ClearCallback() {
		if (ref != LUA_NOREF) {
			lua_unref(T, ref);
			lua_unref(T, threadRef);
			T = nullptr;
			threadRef = LUA_NOREF;
			ref = LUA_NOREF;
		}
	}

	int64_t TestMethod(const char *arg1, int64_t arg2) {
		if (ref != LUA_NOREF) {
			lua_getref(T, ref);
			lua_pushstring(T, arg1);
			lua_pushinteger(T, arg2);
			luaSBX_pcall(T, 2, 0, 0);
		}

		Emit<TestDerived>("TestSignal", arg1, arg2);
		return arg2;
	}

	static int TestLuauMethod(lua_State *L) {
		lua_pushstring(L, "abc");
		lua_pushnumber(L, 123);
		return 2;
	}

	const char *GetStr() const { return str.c_str(); }
	void SetStr(const char *newStr) {
		str = newStr;
		Changed<TestDerived>("Str");
	}

	int GetNum() const { return str.size(); }

	DataTypes::EnumAxis GetAxis() const { return axis; }
	void SetAxis(DataTypes::EnumAxis value) { axis = value; }

	DataTypes::EnumSignalBehavior GetBehavior() const { return behavior; }

	void SetCallback(lua_State *L) {
		ClearCallback();
		if (!lua_isnil(L, 1)) {
			lua_pushthread(L);
			T = L;
			threadRef = lua_ref(L, -1);
			lua_pop(L, 1);
			ref = lua_ref(L, 1);
		}
	}

protected:
	template <typename T>
	static void BindMembers() {
		Object::BindMembers<T>();

		ClassDB::BindMethod<T, "TestMethod", &TestDerived::TestMethod, InternalTestSecurity, ThreadSafety::Unsafe>({}, "arg1", "arg2");
		ClassDB::BindLuauMethod<T, "TestLuauMethod", std::tuple<std::string, int>(int, std::optional<int>), TestDerived::TestLuauMethod, NoneSecurity, ThreadSafety::Safe>({}, "arg1", "arg2");
		ClassDB::BindProperty<T, "Str", "Test Category", &TestDerived::GetStr, InternalTestSecurity, &TestDerived::SetStr, InternalTestSecurity, ThreadSafety::Unsafe, true, true>({ MemberTag::NotBrowsable });
		ClassDB::BindPropertyReadOnly<T, "Num", "Test Category", &TestDerived::GetNum, InternalTestSecurity, ThreadSafety::Unsafe, false>({ MemberTag::NotBrowsable });
		ClassDB::BindPropertyNotScriptable<T, "Axis", "Test Category", &TestDerived::GetAxis, &TestDerived::SetAxis, ThreadSafety::Unsafe, true, true>({});
		ClassDB::BindPropertyNotScriptableReadOnly<T, "Behavior", "Test Category", &TestDerived::GetBehavior, ThreadSafety::ReadSafe, true>({});
		ClassDB::BindSignal<T, "TestSignal", void(const char *, int64_t), InternalTestSecurity>({ MemberTag::Deprecated }, "arg1", "arg2");
		ClassDB::BindCallback<T, "TestCallback", void(const char *, int64_t), &TestDerived::SetCallback, InternalTestSecurity, ThreadSafety::Unsafe>({ MemberTag::NoYield }, "arg1", "arg2");
	}

private:
	std::string str;
	DataTypes::EnumAxis axis = DataTypes::EnumAxis::X;
	DataTypes::EnumSignalBehavior behavior = DataTypes::EnumSignalBehavior::Deferred;

	lua_State *T = nullptr;
	int threadRef = LUA_NOREF;
	int ref = LUA_NOREF;
};

} //namespace SBX::Classes

TEST_CASE("runtime type information") {
	Object::InitializeClass();
	TestDerived::InitializeClass();

	SUBCASE("class metadata") {
		const ClassDB::ClassInfo *info = ClassDB::GetClass("TestDerived");
		REQUIRE_NE(info, nullptr);

		std::unordered_set<ClassTag> tags;

		CHECK_EQ(info->name, "TestDerived");
		CHECK_EQ(info->parent, "Object");
		CHECK_EQ(info->category, MemoryCategory::Internal);
		CHECK_EQ(info->tags, std::unordered_set<ClassTag>{ ClassTag::NotReplicated });
	}

	SUBCASE("method") {
		const ClassDB::Function *func = ClassDB::GetFunction("TestDerived", "TestMethod");
		REQUIRE_NE(func, nullptr);

		CHECK_EQ(func->name, "TestMethod");
		CHECK_EQ(func->returnType, std::vector<std::string>{ "int64" });

		REQUIRE_EQ(func->parameters.size(), 2);
		CHECK_EQ(func->parameters[0].name, "arg1");
		CHECK_EQ(func->parameters[0].type, "string");
		CHECK_EQ(func->parameters[1].name, "arg2");
		CHECK_EQ(func->parameters[1].type, "int64");

		CHECK(func->tags.empty());

		CHECK_EQ(func->security, InternalTestSecurity);
		CHECK_EQ(func->safety, ThreadSafety::Unsafe);
	}

	SUBCASE("Luau method") {
		const ClassDB::Function *func = ClassDB::GetFunction("TestDerived", "TestLuauMethod");
		REQUIRE_NE(func, nullptr);

		CHECK_EQ(func->name, "TestLuauMethod");
		CHECK_EQ(func->returnType, std::vector<std::string>{ "string", "int" });

		REQUIRE_EQ(func->parameters.size(), 2);
		CHECK_EQ(func->parameters[0].name, "arg1");
		CHECK_EQ(func->parameters[0].type, "int");
		CHECK_EQ(func->parameters[1].name, "arg2");
		CHECK_EQ(func->parameters[1].type, "int?");

		CHECK_EQ(func->tags, std::unordered_set<MemberTag>{ MemberTag::CustomLuaState });

		CHECK_EQ(func->security, NoneSecurity);
		CHECK_EQ(func->safety, ThreadSafety::Safe);
	}

	SUBCASE("property") {
		const ClassDB::Property *prop = ClassDB::GetProperty("TestDerived", "Str");
		REQUIRE_NE(prop, nullptr);

		CHECK_EQ(prop->name, "Str");
		CHECK_EQ(prop->changedSignal, "StrChanged");
		CHECK_EQ(prop->category, "Test Category");
		CHECK_NE(prop->getter, nullptr);
		CHECK_NE(prop->setter, nullptr);
		CHECK_EQ(prop->type, "string");
		CHECK_EQ(prop->tags, std::unordered_set<MemberTag>{ MemberTag::NotBrowsable });
		CHECK_EQ(prop->readSecurity, InternalTestSecurity);
		CHECK_EQ(prop->writeSecurity, InternalTestSecurity);
		CHECK_EQ(prop->safety, ThreadSafety::Unsafe);
		CHECK(prop->canLoad);
		CHECK(prop->canSave);
	}

	SUBCASE("property signal") {
		const ClassDB::Signal *sig = ClassDB::GetSignal("TestDerived", "StrChanged");
		REQUIRE_NE(sig, nullptr);

		CHECK_EQ(sig->name, "StrChanged");
		CHECK(sig->parameters.empty());
		CHECK(sig->tags.empty());
		CHECK_EQ(sig->security, InternalTestSecurity);
		CHECK(sig->unlisted);
	}

	SUBCASE("property read-only") {
		const ClassDB::Property *prop = ClassDB::GetProperty("TestDerived", "Num");
		REQUIRE_NE(prop, nullptr);

		CHECK_EQ(prop->name, "Num");
		CHECK_EQ(prop->changedSignal, "NumChanged");
		CHECK_EQ(prop->category, "Test Category");
		CHECK_NE(prop->getter, nullptr);
		CHECK_EQ(prop->setter, nullptr);
		CHECK_EQ(prop->type, "int");
		CHECK_EQ(prop->tags, std::unordered_set<MemberTag>{ MemberTag::ReadOnly, MemberTag::NotBrowsable });
		CHECK_EQ(prop->readSecurity, InternalTestSecurity);
		CHECK_EQ(prop->writeSecurity, NoneSecurity);
		CHECK_EQ(prop->safety, ThreadSafety::Unsafe);
		CHECK_FALSE(prop->canLoad);
		CHECK_FALSE(prop->canSave);
	}

	SUBCASE("property not scriptable") {
		const ClassDB::Property *prop = ClassDB::GetProperty("TestDerived", "Axis");
		REQUIRE_NE(prop, nullptr);

		CHECK_EQ(prop->name, "Axis");
		CHECK(prop->changedSignal.empty());
		CHECK_EQ(prop->category, "Test Category");
		CHECK_NE(prop->getter, nullptr);
		CHECK_NE(prop->setter, nullptr);
		CHECK_EQ(prop->type, "Axis");
		CHECK_EQ(prop->tags, std::unordered_set<MemberTag>{ MemberTag::NotScriptable });
		CHECK_EQ(prop->readSecurity, NoneSecurity);
		CHECK_EQ(prop->writeSecurity, NoneSecurity);
		CHECK_EQ(prop->safety, ThreadSafety::Unsafe);
		CHECK(prop->canLoad);
		CHECK(prop->canSave);
	}

	SUBCASE("property not scriptable read-only") {
		const ClassDB::Property *prop = ClassDB::GetProperty("TestDerived", "Behavior");
		REQUIRE_NE(prop, nullptr);

		CHECK_EQ(prop->name, "Behavior");
		CHECK(prop->changedSignal.empty());
		CHECK_EQ(prop->category, "Test Category");
		CHECK_NE(prop->getter, nullptr);
		CHECK_EQ(prop->setter, nullptr);
		CHECK_EQ(prop->type, "SignalBehavior");
		CHECK_EQ(prop->tags, std::unordered_set<MemberTag>{ MemberTag::NotScriptable, MemberTag::ReadOnly });
		CHECK_EQ(prop->readSecurity, NoneSecurity);
		CHECK_EQ(prop->writeSecurity, NoneSecurity);
		CHECK_EQ(prop->safety, ThreadSafety::ReadSafe);
		CHECK_FALSE(prop->canLoad);
		CHECK(prop->canSave);
	}

	SUBCASE("signal") {
		const ClassDB::Signal *sig = ClassDB::GetSignal("TestDerived", "TestSignal");
		REQUIRE_NE(sig, nullptr);

		CHECK_EQ(sig->name, "TestSignal");

		CHECK_EQ(sig->parameters.size(), 2);
		CHECK_EQ(sig->parameters[0].name, "arg1");
		CHECK_EQ(sig->parameters[0].type, "string");
		CHECK_EQ(sig->parameters[1].name, "arg2");
		CHECK_EQ(sig->parameters[1].type, "int64");

		CHECK_EQ(sig->tags, std::unordered_set<MemberTag>{ MemberTag::Deprecated });
		CHECK_EQ(sig->security, InternalTestSecurity);
		CHECK_FALSE(sig->unlisted);
	}

	SUBCASE("callback") {
		const ClassDB::Callback *cb = ClassDB::GetCallback("TestDerived", "TestCallback");
		REQUIRE_NE(cb, nullptr);

		CHECK_EQ(cb->name, "TestCallback");
		CHECK_NE(cb->func, nullptr);
		CHECK_EQ(cb->returnType, std::vector<std::string>{ "null" });

		CHECK_EQ(cb->parameters.size(), 2);
		CHECK_EQ(cb->parameters[0].name, "arg1");
		CHECK_EQ(cb->parameters[0].type, "string");
		CHECK_EQ(cb->parameters[1].name, "arg2");
		CHECK_EQ(cb->parameters[1].type, "int64");

		CHECK_EQ(cb->tags, std::unordered_set<MemberTag>{ MemberTag::NoYield });
		CHECK_EQ(cb->security, InternalTestSecurity);
		CHECK_EQ(cb->safety, ThreadSafety::Unsafe);
	}
}

TEST_CASE("reflection") {
	Object::InitializeClass();
	TestDerived::InitializeClass();

	std::shared_ptr<TestDerived> testDerived = std::make_shared<TestDerived>();

	CHECK(testDerived->Set<const char *>("Str", "Hello"));
	CHECK_EQ(testDerived->Get<const char *>("Str"), std::string("Hello"));

	CHECK(testDerived->Set<DataTypes::EnumAxis>("Axis", DataTypes::EnumAxis::Y));
	CHECK_EQ(testDerived->Get<DataTypes::EnumAxis>("Axis"), DataTypes::EnumAxis::Y);

	CHECK(testDerived->Set<uint32_t>("Axis", 2));
	CHECK_EQ(testDerived->GetAxis(), DataTypes::EnumAxis::Z);

	CHECK_FALSE(testDerived->Set<int>("Str", 5));
	CHECK_FALSE(testDerived->Get<int>("Str"));

	std::shared_ptr<Object> obj = ClassDB::New("Object");
	CHECK(!obj);
	obj = ClassDB::New("TestDerived");
	CHECK(!!obj);
}

TEST_CASE("stack operation") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	Object::InitializeClass();
	TestDerived::InitializeClass();
	ClassDB::Register(L);

	using Ref = std::shared_ptr<TestDerived>;

	Ref testDerived = std::make_shared<TestDerived>();

	LuauStackOp<Ref>::Push(L, testDerived);
	CHECK_EQ(testDerived.use_count(), 2);

	// Registry use; generic object stack operator
	LuauStackOp<std::shared_ptr<Object>>::Push(L, testDerived);
	CHECK_EQ(testDerived.use_count(), 2);
	CHECK_EQ(luaL_typename(L, -1), std::string("TestDerived"));
	lua_pop(L, 1);

	lua_pop(L, 1);
	lua_gc(L, LUA_GCCOLLECT, 0);
	CHECK_EQ(testDerived.use_count(), 1);

	// Null
	LuauStackOp<std::shared_ptr<Object>>::Push(L, std::shared_ptr<Object>());
	CHECK(lua_isnil(L, -1));
	lua_pop(L, 1);

	luaSBX_close(L);
}

TEST_CASE("Luau API") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	DataTypes::luaSBX_opendatatypes(L);
	Object::InitializeClass();
	TestDerived::InitializeClass();
	ClassDB::Register(L);

	SignalConnectionOwner connections;

	SbxThreadData *udata = luaSBX_getthreaddata(L);
	udata->signalConnections = &connections;

	std::shared_ptr<TestDerived> testDerived = std::make_shared<TestDerived>();
	LuauStackOp<std::shared_ptr<TestDerived>>::Push(L, testDerived);
	lua_setglobal(L, "inst");

	SUBCASE("method") {
		CHECK_EVAL_EQ(L, "return inst:TestMethod('asdf', 1234)", int64_t, 1234);
	}

	SUBCASE("Luau method") {
		CHECK_EVAL_OK(L, "local x, y = inst:TestLuauMethod(); assert(x == 'abc'); assert(y == 123)");
	}

	SUBCASE("property") {
		CHECK_EVAL_OK(L, "inst.Str = 'qwerty'");
		CHECK_EQ(testDerived->GetStr(), std::string("qwerty"));
		CHECK_EVAL_EQ(L, "return inst.Str", std::string, "qwerty");
	}

	SUBCASE("read-only property") {
		CHECK_EVAL_OK(L, "inst.Str = 'qwerty'");
		CHECK_EVAL_EQ(L, "return inst.Num", int, 6);
		CHECK_EVAL_FAIL(L, "inst.Num = 5", "exec:1: Num member of TestDerived is read-only and cannot be assigned to");
	}

	SUBCASE("not scriptable property read") {
		CHECK_EVAL_FAIL(L, "return inst.Axis", "exec:1: Axis is not a valid member of TestDerived");
	}

	SUBCASE("not scriptable property write") {
		CHECK_EVAL_FAIL(L, "inst.Axis = 0", "exec:1: Axis is not a valid member of TestDerived");
	}

	SUBCASE("read-only not scriptable property read") {
		CHECK_EVAL_FAIL(L, "return inst.Behavior", "exec:1: Behavior is not a valid member of TestDerived");
	}

	SUBCASE("read-only not scriptable property write") {
		CHECK_EVAL_FAIL(L, "inst.Behavior = 0", "exec:1: Behavior is not a valid member of TestDerived");
	}

	SUBCASE("property signals") {
		SUBCASE("working") {
			CHECK_EVAL_OK(L, R"ASDF(
				changedHits = 0
				propHits = 0

				inst.Changed:Connect(function(prop)
					if prop == "Str" then changedHits += 1 end
				end)

				inst:GetPropertyChangedSignal("Str"):Connect(function()
					propHits += 1
				end)
			)ASDF");

			testDerived->SetStr("ishdgiahsdg");

			lua_getglobal(L, "changedHits");
			CHECK_EQ(lua_tonumber(L, -1), 1);
			lua_pop(L, 1);

			lua_getglobal(L, "propHits");
			CHECK_EQ(lua_tonumber(L, -1), 1);
			lua_pop(L, 1);
		}

		SUBCASE("non-existent") {
			CHECK_EVAL_FAIL(L, "inst:GetPropertyChangedSignal('oashdgoiahdsoighaoidsf')", "exec:1: oashdgoiahdsoighaoidsf is not a valid property name.");
		}

		SUBCASE("not scriptable") {
			CHECK_EVAL_FAIL(L, "inst:GetPropertyChangedSignal('Behavior')", "exec:1: Behavior is not a valid property name.");
		}

		SUBCASE("no permission") {
			udata->identity = GameScriptIdentity;
			CHECK_EVAL_FAIL(L, "inst:GetPropertyChangedSignal('Str')", "exec:1: The current thread cannot read 'Str' (lacking capability InternalTest)");
		}
	}

	SUBCASE("signal and callback") {
		CHECK_EVAL_OK(L, R"ASDF(
			a = nil
			b = nil
			c = nil
			d = nil

			inst.TestSignal:Connect(function(arg1, arg2)
				a = arg1
				b = arg2
			end)

			inst.TestCallback = function(arg1, arg2)
				c = arg1
				d = arg2
			end
		)ASDF");

		testDerived->TestMethod("asdf", 1234);

		lua_getglobal(L, "a");
		CHECK_EQ(lua_tostring(L, -1), std::string("asdf"));
		lua_pop(L, 1);

		lua_getglobal(L, "c");
		CHECK_EQ(lua_tostring(L, -1), std::string("asdf"));
		lua_pop(L, 1);

		lua_getglobal(L, "b");
		CHECK_EQ(lua_tonumber(L, -1), 1234);
		lua_pop(L, 1);

		lua_getglobal(L, "d");
		CHECK_EQ(lua_tonumber(L, -1), 1234);
		lua_pop(L, 1);

		CHECK_EVAL_OK(L, "inst.TestCallback = nil");

		testDerived->TestMethod("qwer", 5678);

		lua_getglobal(L, "c");
		CHECK_EQ(lua_tostring(L, -1), std::string("asdf"));
		lua_pop(L, 1);

		lua_getglobal(L, "d");
		CHECK_EQ(lua_tonumber(L, -1), 1234);
		lua_pop(L, 1);
	}

	luaSBX_close(L);
}

TEST_CASE("Object Luau API") {
	lua_State *L = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	Object::InitializeClass();
	TestDerived::InitializeClass();
	ClassDB::Register(L);

	using Ref = std::shared_ptr<TestDerived>;

	Ref testDerived = std::make_shared<TestDerived>();

	LuauStackOp<Ref>::Push(L, testDerived);
	lua_setglobal(L, "obj");

	SUBCASE("ClassName") {
		CHECK_EVAL_EQ(L, "return obj.ClassName", std::string, "TestDerived");
		CHECK_EVAL_EQ(L, "return obj.className", std::string, "TestDerived");
	}

	SUBCASE("ClassName read-only") {
		CHECK_EVAL_FAIL(L, "obj.ClassName = 1", "exec:1: ClassName member of TestDerived is read-only and cannot be assigned to");
	}

	SUBCASE("IsA") {
		CHECK_EVAL_EQ(L, "return obj:IsA('TestDerived')", bool, true);
		CHECK_EVAL_EQ(L, "return obj:IsA('Object')", bool, true);
		CHECK_EVAL_EQ(L, "return obj:isA('Object')", bool, true);
		CHECK_EVAL_EQ(L, "return obj:IsA('aoihdoigahsoidhgoiasdag')", bool, false);
	}

	luaSBX_close(L);
}

TEST_SUITE_END();
