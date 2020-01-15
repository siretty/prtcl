#pragma once

#include <sstream>

#include <boost/container/small_vector.hpp>

#include <boost/range/adaptor/indexed.hpp>

namespace prtcl::core {

using nd_shape = boost::container::small_vector<size_t, 2>;

template <typename Range> auto make_nd_shape(Range &&range_) {
  return nd_shape{std::begin(std::forward<Range>(range_)),
                  std::end(std::forward<Range>(range_))};
}

inline std::string to_string(nd_shape shape_) {
  std::ostringstream ss;
  for (auto v : shape_ | boost::adaptors::indexed()) {
    if (0 != v.index())
      ss << ", ";
    ss << v.value();
  }
  return ss.str();
}

} // namespace prtcl::gt
