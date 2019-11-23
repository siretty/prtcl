#pragma once

#include "prtcl/meta/remove_cvref.hpp"
#include <prtcl/expr/reduction.hpp>

#include <ostream>
#include <utility>

namespace prtcl::scheme {

enum class eq_kind { normal, reduce };

inline std::ostream &operator<<(std::ostream &s, eq_kind kind) {
  switch (kind) {
  case eq_kind::normal:
    s << "prtcl::scheme::eq_kind::normal";
    break;
  case eq_kind::reduce:
    s << "prtcl::scheme::eq_kind::reduce";
    break;
  default:
    s << "prtcl::scheme::eq_kind(" << static_cast<int>(kind) << ")";
    break;
  }
  return s;
}

using eq_operator = prtcl::expr::reduction_op;

template <eq_kind Kind, eq_operator Op, typename LHS, typename RHS> struct eq {
  //static_assert(Kind == eq_kind::normal or Op != eq_operator::none);

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the tuple of transformed expressions
    using lhs_type = decltype(std::declval<Transform>()(std::declval<LHS>()));
    using rhs_type = decltype(std::declval<Transform>()(std::declval<RHS>()));
    // return the resulting reduction
    return eq<Kind, Op, lhs_type, rhs_type>{
        std::forward<Transform>(transform)(lhs),
        std::forward<Transform>(transform)(rhs)};
  }

  LHS lhs;
  RHS rhs;
};

template <eq_operator Op, typename LHS, typename RHS>
auto make_reduce_eq(LHS &&lhs, RHS &&rhs) {
  return eq<eq_kind::reduce, Op, meta::remove_cvref_t<LHS>,
            meta::remove_cvref_t<RHS>>{std::forward<LHS>(lhs),
                                       std::forward<RHS>(rhs)};
}

template <typename LHS, typename RHS>
auto make_min_reduce_eq(LHS &&lhs, RHS &&rhs) {
  return make_reduce_eq<eq_operator::min>(std::forward<LHS>(lhs),
                                          std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
auto make_max_reduce_eq(LHS &&lhs, RHS &&rhs) {
  return make_reduce_eq<eq_operator::max>(std::forward<LHS>(lhs),
                                          std::forward<RHS>(rhs));
}

} // namespace prtcl::scheme
