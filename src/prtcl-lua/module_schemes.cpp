#include "module_schemes.hpp"

#include <prtcl/log.hpp>
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
    t.set_function("get_full_name", &SchemeBase::GetFullName);
    t.set_function("get_procedure_names", &SchemeBase::GetProcedureNames);
    t.set_function("get_prtcl_source_code", &SchemeBase::GetPrtclSourceCode);

    t.set_function(
        "run_procedure", [](SchemeBase &scheme, std::string_view name,
                            Neighborhood const &nhood) {
          log::Debug(
              "lua", "scheme", scheme.GetFullName(), " RunProcedure('", name,
              "') [[[");
          scheme.RunProcedure(name, nhood);
          log::Debug(
              "lua", "scheme", scheme.GetFullName(), " RunProcedure('", name,
              "') ]]]");
        });
  }

  return m;
}

} // namespace prtcl::lua
