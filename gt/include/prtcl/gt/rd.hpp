#pragma once

#include <prtcl/gt/field.hpp>

#include <boost/yap/yap.hpp>

namespace prtcl::gt {

enum class rd_kind { plus, minus, multiplies, divides, maximum, minimum };

template <typename Expr_> struct rd {
  static_assert(::boost::yap::is_expr<Expr_>::value);

  rd_kind kind;
  field field;
  Expr_ expression;
};

} // namespace prtcl::gt
