#ifndef PRTCL_MATH_COMMON_HPP
#define PRTCL_MATH_COMMON_HPP

#include "constants.hpp"
#include "impl.hpp"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace prtcl::math {

// ============================================================================
// Operations
// ============================================================================

template <typename LHS, typename RHS>
decltype(auto) outer_product(LHS &&lhs, RHS &&rhs) {
  return matmul(std::forward<LHS>(lhs), transpose(std::forward<RHS>(rhs)));
}

template <typename... Arg>
decltype(auto) max(Arg &&... arg) {
  using type = std::common_type_t<Arg...>;
  return std::max(std::initializer_list<type>{std::forward<Arg>(arg)...});
}

template <typename... Arg>
decltype(auto) min(Arg &&... arg) {
  using type = std::common_type_t<Arg...>;
  return std::min(std::initializer_list<type>{std::forward<Arg>(arg)...});
}

} // namespace prtcl::math

#endif // PRTCL_MATH_COMMON_HPP
