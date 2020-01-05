#pragma once

#include <prtcl/gt/field.hpp>

#define PRTCL_DEFINE_FIELD_ALIASES(Name, Kind, Type, Shape)                    \
  namespace prtcl::gt {                                                        \
  using Name##_field = ::prtcl::gt::Shape##_field<                             \
      ::prtcl::gt::field_kind::Kind, ::prtcl::gt::field_type::Type>;           \
  } /* namespace prtcl::gt */                                                  \
  namespace prtcl::gt::field_literals {                                        \
  inline auto operator""_##Name##f(char const *ptr, size_t len) {              \
    return Name##_field{{std::string_view{ptr, len}}};                         \
  }                                                                            \
  inline auto operator""_##Name(char const *ptr, size_t len) {                 \
    return ::boost::yap::make_terminal(                                        \
        Name##_field{{std::string_view{ptr, len}}});                           \
  }                                                                            \
  } /* namespace prtcl::gt::field_literals */

PRTCL_DEFINE_FIELD_ALIASES(grs, global, real, scalar)
PRTCL_DEFINE_FIELD_ALIASES(grv, global, real, vector)
PRTCL_DEFINE_FIELD_ALIASES(grm, global, real, matrix)

PRTCL_DEFINE_FIELD_ALIASES(urs, uniform, real, scalar)
PRTCL_DEFINE_FIELD_ALIASES(urv, uniform, real, vector)
PRTCL_DEFINE_FIELD_ALIASES(urm, uniform, real, matrix)

PRTCL_DEFINE_FIELD_ALIASES(vrs, varying, real, scalar)
PRTCL_DEFINE_FIELD_ALIASES(vrv, varying, real, vector)
PRTCL_DEFINE_FIELD_ALIASES(vrm, varying, real, matrix)

#undef PRTCL_DEFINE_FIELD_ALIASES
