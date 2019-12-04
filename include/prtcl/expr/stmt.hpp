#pragma once

#include <prtcl/expr/gfield.hpp>
//#include <prtcl/expr/ufield.hpp>
//#include <prtcl/expr/vfield.hpp>

#include <type_traits>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

constexpr bool is_assign(boost::yap::expr_kind kind) {
  return kind == boost::yap::expr_kind::assign;
}

constexpr bool is_opassign(boost::yap::expr_kind kind) {
  return kind == boost::yap::expr_kind::plus_assign ||
         kind == boost::yap::expr_kind::minus_assign ||
         kind == boost::yap::expr_kind::multiplies_assign ||
         kind == boost::yap::expr_kind::divides_assign;
}

struct nl_stmt_xform {
  template <boost::yap::expr_kind Kind, typename TT, typename V, typename RHS,
            typename = std::enable_if_t<is_assign(Kind) || is_opassign(Kind)>>
  decltype(auto)
  operator()(boost::yap::expression<
             Kind, boost::hana::tuple<
                       boost::yap::expression<boost::yap::expr_kind::terminal,
                                              gfield<TT, V>>,
                       RHS>> const &expr) {
    (void)(expr); //
  }
};

template <typename Expr> auto nl_stmt(Expr const &expr) {
  return boost::yap::transform_strict(expr, nl_stmt_xform{});
}

} // namespace prtcl::expr
