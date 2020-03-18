#pragma once

#include <string>
#include <type_traits>

namespace prtcl::core {

enum class dtype {
  real,
  integer,
  boolean,
};

inline std::string to_string(dtype v_) {
  switch (v_) {
  case dtype::real:
    return "real";
  case dtype::integer:
    return "integer";
  case dtype::boolean:
    return "boolean";
  default:
    return "dtype(" +
           std::to_string(static_cast<std::underlying_type_t<dtype>>(v_)) + ")";
  }
}

} // namespace prtcl::core
