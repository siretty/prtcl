#include "main.hpp"

#include "entry_point.hpp"

#include <prtcl/cxx.hpp>
#include <prtcl/log.hpp>

#include <sstream>
#include <string>
#include <vector>

#include <sol/sol.hpp>

namespace prtcl::lua {
namespace {

static std::string const default_script = R"==(
local prtcl = require 'prtcl'

prtcl.log:errorf("default_script", "no file specified")
)==";

static int Main(cxx::span<std::string const> argv) {
  ::prtcl::log::Debug(
      "app", "prtcl-lua", "prtcl::lua::main(#argv=", argv.size(), ")");

  sol::state lua;
  lua.open_libraries(
      sol::lib::base, sol::lib::string, sol::lib::package, sol::lib::math,
      sol::lib::io);

  // preload our own package (builtin)
  lua["package"]["preload"]["prtcl"] = luaopen_prtcl;

  // setup the args table
  lua["args"] = sol::as_table(argv);

  // setup the print function to pass through to log
  lua["print"] = [](sol::this_state s, sol::variadic_args args) {
    sol::state_view lua{s};
    std::ostringstream ss;
    for (size_t argi = 0; argi < args.size(); ++argi) {
      if (argi >= 1)
        ss << '\t';
      ss << lua["tostring"](args[argi]).get<std::string>();
    }
    log::Info("lua", "print", ss.str());
  };

  // load and execute the script file
  if (argv.size() >= 2)
    lua.script_file(argv[1]);
  else
    lua.script(default_script);

  return 0;
}

} // namespace
} // namespace prtcl::lua

int main(int argc, char **argv_ptr) {
  std::vector<std::string> argv;
  for (int argi = 0; argi < argc; ++argi)
    argv.push_back({argv_ptr[argi]});

  return ::prtcl::lua::Main(argv);
}
