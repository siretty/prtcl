#pragma once

#include <prtcl/scheme/compiler/block.hpp>

#include <vector>

#include <boost/hana.hpp>

namespace prtcl::scheme {

template <typename Select, typename Block> struct unbound_loop {
  static_assert(is_block<Block>::value);

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the tuple of transformed block
    using transformed_block =
        decltype(std::declval<Block>().transform(std::declval<Transform>()));
    // build the resulting loop
    return unbound_loop<Select, transformed_block>{
        select, block.transform(std::forward<Transform>(transform))};
  }

  Select select;
  Block block;
};

template <typename> struct is_unbound_loop : std::false_type {};
template <typename S, typename B>
struct is_unbound_loop<unbound_loop<S, B>> : std::true_type {};

} // namespace prtcl::scheme
