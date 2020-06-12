//
// Created by daned on 6/12/20.
//

#include "entry_point.hpp"

#include <prtcl/log.hpp>

#include <prtcl/data/group.hpp>
#include <prtcl/data/model.hpp>
#include <prtcl/data/uniform_manager.hpp>
#include <prtcl/data/varying_manager.hpp>

#include <sol/sol.hpp>

namespace prtcl::lua {
namespace {

static auto ModuleLog(sol::state_view lua) {
  auto m = lua.create_table();

  m["debugf"] = [](sol::this_state s, sol::object self, std::string origin,
                   sol::variadic_args args) {
    sol::state_view lua{s};
    std::string message = lua["string"]["format"](args);
    ::prtcl::log::Debug("lua", origin, message);
    return self;
  };

  m["infof"] = [](sol::this_state s, sol::object self, std::string origin,
                  sol::variadic_args args) {
    sol::state_view lua{s};
    std::string message = lua["string"]["format"](args);
    ::prtcl::log::Info("lua", origin, message);
    return self;
  };

  m["warningf"] = [](sol::this_state s, sol::object self, std::string origin,
                     sol::variadic_args args) {
    sol::state_view lua{s};
    std::string message = lua["string"]["format"](args);
    ::prtcl::log::Warning("lua", origin, message);
    return self;
  };

  m["errorf"] = [](sol::this_state s, sol::object self, std::string origin,
                   sol::variadic_args args) {
    sol::state_view lua{s};
    std::string message = lua["string"]["format"](args);
    ::prtcl::log::Error("lua", origin, message);
    return self;
  };

  return m;
}

static auto ModuleData(sol::state_view lua) {
  auto m = lua.create_table();

  auto model_type =
      m.new_usertype<Model>("model", sol::constructors<Model()>());
  model_type["add_group"] = &Model::AddGroup;
  model_type["get_group"] = &Model::TryGetGroup;
  model_type["remove_group"] = &Model::RemoveGroup;
  model_type["group_count"] = sol::property(&Model::GetGroupCount);
  model_type["global"] = sol::property(&Model::GetGlobal);

  auto group_type = m.new_usertype<Group>("group", sol::no_constructor);
  group_type["group_name"] = sol::property(&Group::GetGroupName);
  group_type["group_type"] = sol::property(&Group::GetGroupType);
  group_type["group_index"] = sol::property(&Group::GetGroupIndex);
  group_type["item_count"] = sol::property(&Group::GetItemCount);
  group_type["uniform"] = sol::property(&Group::GetUniform);
  group_type["varying"] = sol::property(&Group::GetVarying);

  {
    auto t =
        m.new_usertype<UniformManager>("uniform_manager", sol::no_constructor);
    t["field_count"] = sol::property(&UniformManager::GetFieldCount);
    t["get_field"] = &UniformManager::TryGetField;
    t["has_field"] = &UniformManager::HasField;
  }

  {
    auto t =
        m.new_usertype<VaryingManager>("varying_manager", sol::no_constructor);
    t["field_count"] = sol::property(&VaryingManager::GetFieldCount);
    t["get_field"] = &VaryingManager::TryGetField;
    t["has_field"] = &VaryingManager::HasField;
  }

  return m;
}

static auto ModuleEntryPoint(sol::state_view lua) {
  auto m = lua.create_table();
  m["log"] = ModuleLog(lua);
  m["data"] = ModuleData(lua);
  return m;
}

static int RawModuleEntryPoint(lua_State *L) {
  sol::state_view lua{L};

  // TODO: add error and exception handling
  auto m = ModuleEntryPoint(lua);

  sol::stack::push(lua.lua_state(), m);
  return 1;
}

} // namespace
} // namespace prtcl::lua

extern "C" {

int luaopen_libprtcl_lua(lua_State *L) {
  ::prtcl::log::Debug(
      "app", "prtcl-lua", "luaopen_libprtcl_lua(", static_cast<void *>(L), ")");

  return ::prtcl::lua::RawModuleEntryPoint(L);
}

int luaopen_prtcl(lua_State *L) {
  ::prtcl::log::Debug(
      "app", "prtcl-lua", "luaopen_prtcl(", static_cast<void *>(L), ")");

  return luaopen_libprtcl_lua(L);
}

} // extern "C"
