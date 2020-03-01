#pragma once

#include <boost/container/flat_set.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace prtcl::core {

template <typename Range_> auto make_set(Range_ &&range) {
  using value_type = typename Range_::value_type;
  return boost::container::flat_set<value_type>{boost::begin(range),
                                                boost::end(range)};
}

} // namespace prtcl::core
