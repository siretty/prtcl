#pragma once

#include "prtcl/meta/remove_cvref.hpp"

#include <ostream>
#include <utility>

#include <boost/yap/yap.hpp>

namespace prtcl::scheme {

template <typename Expr> struct stmt {
  static_assert(boost::yap::is_expr<Expr>::value);

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the tuple of transformed expressions
    using transformed_expr =
        decltype(boost::yap::transform(expr, std::declval<Transform>()));
    // return the resulting reduction
    return stmt<transformed_expr>{
        boost::yap::transform(expr, std::forward<Transform>(transform))};
  }

  Expr expr;
};

} // namespace prtcl::scheme
