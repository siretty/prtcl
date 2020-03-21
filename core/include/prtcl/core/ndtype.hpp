#pragma once

#include "dtype.hpp"

#include <vector>

#include <boost/range/algorithm/equal.hpp>

namespace prtcl::core {

struct ndtype {
  dtype type;
  std::vector<unsigned> shape;

  friend bool operator==(ndtype const &lhs, ndtype const &rhs) {
    return (lhs.type == rhs.type) and boost::range::equal(lhs.shape, rhs.shape);
  }

  friend bool operator!=(ndtype const &lhs, ndtype const &rhs) {
    return not(lhs == rhs);
  }
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
