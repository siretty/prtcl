#ifndef PRTCL_SRC_PRTCL_LUA_ENTRY_POINT_HPP
#define PRTCL_SRC_PRTCL_LUA_ENTRY_POINT_HPP

#include <prtcl/cxx.hpp>

extern "C" {

#include <lua.h>

PRTCL_EXPORT int luaopen_libprtcl_lua(lua_State *L);

PRTCL_EXPORT int luaopen_prtcl(lua_State *L);

} // extern "C"

#endif // PRTCL_SRC_PRTCL_LUA_ENTRY_POINT_HPP
