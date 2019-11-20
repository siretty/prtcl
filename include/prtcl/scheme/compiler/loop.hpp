#pragma once

#include <prtcl/scheme/compiler/block.hpp>

#include <vector>

#include <boost/hana.hpp>

namespace prtcl::scheme {

template <typename Block> struct loop {
  static_assert(is_block<Block>::value);

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the tuple of transformed block
    using transformed_block =
        decltype(std::declval<Block>().transform(std::declval<Transform>()));
    // build the resulting loop
    loop<transformed_block> result{groups, {}};
    for (auto &block : instances) {
      result.instances.emplace_back(
          block.transform(std::forward<Transform>(transform)));
    }
    return std::move(result);
  }

  std::vector<size_t> groups;
  std::vector<Block> instances;
};

template <typename> struct is_loop : std::false_type {};
template <typename T> struct is_loop<loop<T>> : std::true_type {};

} // namespace prtcl::scheme
