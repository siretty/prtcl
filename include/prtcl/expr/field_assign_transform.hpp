#pragma once

#include "field.hpp"

#include <cstddef>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

struct field_assign_transform {
private:
  template <typename Arg> auto _eval(Arg &&arg) const {
    return boost::yap::evaluate(boost::yap::transform(
        boost::yap::as_expr(std::forward<Arg>(arg)), *this));
  }

public:
  template <typename KT, typename TT, typename V, typename RHS>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::assign>,
                  field<KT, TT, tag::active, V> const &field_,
                  RHS &&rhs) const {
    field_.value.set(active, _eval(std::forward<RHS>(rhs)));
  }

  template <typename KT, typename TT, typename V, typename RHS>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::plus_assign>,
                  field<KT, TT, tag::active, V> const &field_,
                  RHS &&rhs) const {
    field_.value.set(active,
                     field_.value.get(active) + _eval(std::forward<RHS>(rhs)));
  }

  template <typename KT, typename TT, typename V, typename RHS>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::minus_assign>,
                  field<KT, TT, tag::active, V> const &field_,
                  RHS &&rhs) const {
    field_.value.set(active,
                     field_.value.get(active) - _eval(std::forward<RHS>(rhs)));
  }

  template <typename KT, typename TT, typename V>
  auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
                  field<KT, TT, tag::active, V> const &field_) const {
    return boost::yap::as_expr(field_.value.get(active));
  }

  template <typename KT, typename TT, typename V>
  auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
                  field<KT, TT, tag::passive, V> const &field_) const {
    return boost::yap::as_expr(field_.value.get(passive));
  }

  size_t active, passive;
};

} // namespace prtcl::expr
