#ifndef PRTCL_SRC_PRTCL_LUA_MODULE_DATA_HPP
#define PRTCL_SRC_PRTCL_LUA_MODULE_DATA_HPP

#include <sol/sol.hpp>

namespace prtcl::lua {

sol::table ModuleData(sol::state_view lua);

} // namespace prtcl::lua

#endif // PRTCL_SRC_PRTCL_LUA_MODULE_DATA_HPP
