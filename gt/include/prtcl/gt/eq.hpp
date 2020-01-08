#pragma once

#include <prtcl/gt/field.hpp>

#include <boost/yap/yap.hpp>

namespace prtcl::gt {

enum class eq_op { assign, plus, minus, multiplies, divides, maximum, minimum };

template <typename Expr_> struct eq {
  static_assert(::boost::yap::is_expr<Expr_>::value);

  eq_op op;
  field field;
  Expr_ expression;

  bool is_reduction() const {
    return op != eq_op::assign && field.kind() != field_kind::varying;
  }
};

} // namespace prtcl::gt
