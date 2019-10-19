#pragma once

#include "field.hpp"

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

struct field_access_transform {
  struct ro {
    template <typename KT, typename TT, typename GT, typename V>
    expr::field_term<KT, TT, GT, tag::read, V> operator()(
        boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
        expr::field<KT, TT, GT, tag::unspecified, V> const &field_) const {
      return {{field_.value}};
    }
  };

  struct rw {
    template <typename KT, typename TT, typename GT, typename V>
    expr::field_term<KT, TT, GT, tag::read_write, V> operator()(
        boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
        expr::field<KT, TT, GT, tag::unspecified, V> const &field_) const {
      return {{field_.value}};
    }
  };

  template <typename LHS, typename RHS>
  auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::assign>,
                  LHS &&lhs, RHS &&rhs) const {
    namespace yap = boost::yap;
    return yap::make_expression<yap::expr_kind::assign>(
        yap::transform(yap::as_expr(std::forward<LHS>(lhs)), rw{}),
        yap::transform(yap::as_expr(std::forward<RHS>(rhs)), ro{}));
  }

  template <typename LHS, typename RHS>
  auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::plus_assign>,
                  LHS &&lhs, RHS &&rhs) const {
    namespace yap = boost::yap;
    return yap::make_expression<yap::expr_kind::plus_assign>(
        yap::transform(yap::as_expr(std::forward<LHS>(lhs)), rw{}),
        yap::transform(yap::as_expr(std::forward<RHS>(rhs)), ro{}));
  }

  template <typename LHS, typename RHS>
  auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::minus_assign>,
                  LHS &&lhs, RHS &&rhs) const {
    namespace yap = boost::yap;
    return yap::make_expression<yap::expr_kind::minus_assign>(
        yap::transform(yap::as_expr(std::forward<LHS>(lhs)), rw{}),
        yap::transform(yap::as_expr(std::forward<RHS>(rhs)), ro{}));
  }
};

} // namespace prtcl::expr
