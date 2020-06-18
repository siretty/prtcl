#ifndef PRTCL_SRC_PRTCL_LUA_MODULE_UTIL_HPP
#define PRTCL_SRC_PRTCL_LUA_MODULE_UTIL_HPP

#include <sol/sol.hpp>

namespace prtcl::lua {

sol::table ModuleUtil(sol::state_view lua);

} // namespace prtcl::lua

#endif // PRTCL_SRC_PRTCL_LUA_MODULE_UTIL_HPP
