#pragma once

#include <prtcl/gt/field.hpp>

#include <boost/yap/yap.hpp>

#define PRTCL_DEFINE_FIELD_ALIASES(Name, Kind, Type, Shape)                    \
  namespace prtcl::gt::field_literals {                                        \
  inline auto operator""_##Name(char const *ptr, size_t len) {                 \
    return field{field_kind::Kind, field_type::Type, Shape##_shape_tag{},      \
                 std::string_view{ptr, len}};                                  \
  }                                                                            \
  inline auto operator""_##Name##_term(char const *ptr, size_t len) {          \
    return ::boost::yap::make_terminal(                                        \
        field{field_kind::Kind, field_type::Type, Shape##_shape_tag{},         \
              std::string_view{ptr, len}});                                    \
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
