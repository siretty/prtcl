#pragma once

#include <prtcl/gt/constant.hpp>

#include <boost/yap/yap.hpp>

#define PRTCL_DEFINE_CONSTANT_ALIASES(Name, Type, Shape)                       \
  namespace prtcl::gt::constant_literals {                                     \
  inline auto operator""_##Name(char const *ptr, size_t len) {                 \
    return constant{field_type::Type, Shape##_shape_tag{},                     \
                    std::string_view{ptr, len}};                               \
  }                                                                            \
  inline auto operator""_##Name##_term(char const *ptr, size_t len) {          \
    return ::boost::yap::make_terminal(constant{                               \
        field_type::Type, Shape##_shape_tag{}, std::string_view{ptr, len}});   \
  }                                                                            \
  } /* namespace prtcl::gt::constant_literals */

PRTCL_DEFINE_CONSTANT_ALIASES(crs, real, scalar)
PRTCL_DEFINE_CONSTANT_ALIASES(crv, real, vector)
PRTCL_DEFINE_CONSTANT_ALIASES(crm, real, matrix)

#undef PRTCL_DEFINE_CONSTANT_ALIASES

/* TODO: integrate numeric constants
 
  inline auto operator""_##Name(unsigned long long val) {                      \
    return constant{field_type::Type, Shape##_shape_tag{},                     \
                    std::to_string(val)};                                      \
  }                                                                            \
  inline auto operator""_##Name(long double val) {                             \
    return constant{field_type::Type, Shape##_shape_tag{},                     \
                    std::to_string(val)};                                      \
  }                                                                            \

 */
