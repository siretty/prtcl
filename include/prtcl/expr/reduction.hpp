#pragma once

#include "../tags.hpp"

#include <ostream>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

enum class reduction_op { plus, minus, multiplies, divides, min, max };

inline std::ostream &operator<<(std::ostream &s, reduction_op op) {
  switch (op) {
  case reduction_op::plus:
    s << "plus";
    break;
  case reduction_op::minus:
    s << "minus";
    break;
  case reduction_op::multiplies:
    s << "multiplies";
    break;
  case reduction_op::divides:
    s << "divides";
    break;
  case reduction_op::min:
    s << "min";
    break;
  case reduction_op::max:
    s << "max";
    break;
  default:
    s << "prtcl::expr::reduction_op(" << static_cast<int>(op) << ")";
    break;
  }
  return s;
}

template <reduction_op Op> struct reduction { constexpr static auto op = Op; };

template <reduction_op Op>
using reduction_term =
    boost::yap::terminal<boost::yap::expression, reduction<Op>>;

using min_reduction = reduction_term<reduction_op::min>;
using max_reduction = reduction_term<reduction_op::max>;

} // namespace prtcl::expr
