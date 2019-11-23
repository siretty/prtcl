#pragma once

#include "prtcl/meta/remove_cvref.hpp"
#include <utility>

#include <boost/hana.hpp>

namespace prtcl::scheme {

template <typename... Ts> struct block {
  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the tuple of transformed expressions
    using transformed_statements = decltype(boost::hana::transform(
        std::declval<boost::hana::tuple<Ts...>>(), std::declval<Transform>()));
    // build the resulting loop
    return block<transformed_statements>{
        boost::hana::transform(statements, std::forward<Transform>(transform))};
  }

  boost::hana::tuple<Ts...> statements;
};

template <typename... Ts> auto make_block(Ts &&... ts) {
  return block<meta::remove_cvref_t<Ts>...>{
      boost::hana::make_tuple(std::forward<Ts>(ts)...)};
}

template <typename> struct is_block : std::false_type {};
template <typename... Ts> struct is_block<block<Ts...>> : std::true_type {};

} // namespace prtcl::scheme
