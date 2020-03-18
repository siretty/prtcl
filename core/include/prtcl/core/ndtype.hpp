#pragma once

#include "dtype.hpp"

#include <vector>

namespace prtcl::core {

struct ndtype {
  dtype type;
  std::vector<unsigned> shape;
};

inline std::string to_string(ndtype v_) {
  auto result = to_string(v_.type);
  for (auto extent : v_.shape) {
    result += "[";
    if (extent > 0)
      result += std::to_string(extent);
    result += "]";
  }
  return result;
}

} // namespace prtcl::core
