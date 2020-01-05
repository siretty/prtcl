#pragma once

#include <prtcl/core/invalid_enumerator_error.hpp>

#include <string_view>

namespace prtcl::gt {

enum class field_type {
  real,
  index,
  boolean,
};

inline constexpr std::string_view enumerator_name(field_type type_) {
  switch (type_) {
  case field_type::real:
    return "real";
  case field_type::index:
    return "index";
  case field_type::boolean:
    return "boolean";
  default:
    throw invalid_enumerator_error<field_type>{type_};
  }
}

} // namespace prtcl::gt
