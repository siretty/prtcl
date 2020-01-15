#pragma once

#include <string>
#include <type_traits>

namespace prtcl::core {

enum class nd_dtype {
  real,
  integer,
  boolean,
};

inline std::string to_string(nd_dtype v_) {
  switch (v_) {
  case nd_dtype::real:
    return "real";
  case nd_dtype::integer:
    return "integer";
  case nd_dtype::boolean:
    return "boolean";
  default:
    return "nd_dtype(" +
           std::to_string(static_cast<std::underlying_type_t<nd_dtype>>(v_)) +
           ")";
  }
}

} // namespace prtcl::core
