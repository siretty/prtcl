#pragma once

#include <utility>

#include <boost/range/algorithm/count_if.hpp>
#include <boost/range/algorithm/equal.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace prtcl::core {

// {{{ make_iterator_range(...)

template <typename ForwardRange_>
auto make_iterator_range(ForwardRange_ &range) {
  return boost::make_iterator_range(range);
}

template <typename ForwardRange_>
auto make_iterator_range(ForwardRange_ const &range) {
  return boost::make_iterator_range(range);
}

template <typename ForwardTraversalIterator_>
auto make_iterator_range(
    ForwardTraversalIterator_ first, ForwardTraversalIterator_ last) {
  return boost::make_iterator_range(first, last);
}

// }}}

template <typename Container_, typename Value_>
bool contains(Container_ const &container, Value_ &&value) {
  // TODO: use sfinae / concepts to select this for containers that have a find
  //       method, otherwise iterate through the range (ie. fallback to
  //       std::ranges::find)
  return container.find(std::forward<Value_>(value)) != container.end();
}

namespace ranges {

using boost::range::count_if;
using boost::range::equal;

} // namespace ranges

namespace range_adaptors {

auto const map_keys = boost::adaptors::map_keys;
auto const transformed = boost::adaptors::transformed;
auto const reversed = boost::adaptors::reversed;

} // namespace range_adaptors

} // namespace prtcl::core
