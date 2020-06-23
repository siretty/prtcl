#include "entry_point.hpp"

#include "module_data.hpp"
#include "module_geometry.hpp"
#include "module_math.hpp"
#include "module_schemes.hpp"
#include "module_util.hpp"

#include <prtcl/log.hpp>

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

static auto ModuleEntryPoint(sol::state_view lua) {
  auto m = lua.create_table();
  m["log"] = ModuleLog(lua);
  m["data"] = ModuleData(lua);
  m["geometry"] = ModuleGeometry(lua);
  m["math"] = ModuleMath(lua);
  m["schemes"] = ModuleSchemes(lua);
  m["util"] = ModuleUtil(lua);
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
