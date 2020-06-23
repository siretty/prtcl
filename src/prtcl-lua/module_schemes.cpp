#include "module_schemes.hpp"

#include <prtcl/schemes/scheme_base.hpp>

#include <string>

namespace prtcl::lua {

sol::table ModuleSchemes(sol::state_view lua) {
  auto m = lua.create_table();

  m["get_scheme_names"] = [] { return GetSchemeRegistry().GetSchemeNames(); };

  m.set_function("make", [](std::string scheme_name) {
    return GetSchemeRegistry().NewScheme(scheme_name);
  });

  {
    auto t = m.new_usertype<SchemeBase>("scheme", sol::no_constructor);

    t.set_function("load", &SchemeBase::Load);
    t.set_function("get_procedure_names", &SchemeBase::GetProcedureNames);
    t.set_function("run_procedure", &SchemeBase::RunProcedure);
  }

  return m;
}

} // namespace prtcl::lua
