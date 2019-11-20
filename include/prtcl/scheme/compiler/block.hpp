#pragma once

#include <boost/hana.hpp>

namespace prtcl::scheme {

template <typename Statements> struct block {
  // TODO: assert that statements is a hana sequence of statements

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the tuple of transformed expressions
    using transformed_statements = decltype(boost::hana::transform(
        std::declval<Statements>(), std::declval<Transform>()));
    // build the resulting loop
    return block<transformed_statements>{
        boost::hana::transform(statements, std::forward<Transform>(transform))};
  }

  Statements statements;
};

template <typename> struct is_block : std::false_type {};
template <typename T> struct is_block<block<T>> : std::true_type {};

} // namespace prtcl::scheme
