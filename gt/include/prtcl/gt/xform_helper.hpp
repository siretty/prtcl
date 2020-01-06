#pragma once

#include <boost/hana.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::gt {

struct xform_helper {
protected:
  using expr_kind = boost::yap::expr_kind;

  template <expr_kind K, typename... Ts>
  using expr = boost::yap::expression<K, boost::hana::tuple<Ts...>>;

  template <typename T> using term = expr<expr_kind::terminal, T>;

  template <typename L, typename R>
  using subs = expr<expr_kind::subscript, L, R>;

  template <typename... Ts> using call = expr<expr_kind::call, Ts...>;

  template <expr_kind K>
  static constexpr bool is_assign_v = K == expr_kind::assign;

  template <expr_kind K>
  static constexpr bool is_opassign_v = boost::hana::in(
      K, boost::hana::make_tuple(
             expr_kind::plus_assign, expr_kind::minus_assign,
             expr_kind::multiplies_assign, expr_kind::divides_assign));

  template <expr_kind K>
  static constexpr bool is_uop_v = boost::hana::in(
      K, boost::hana::make_tuple(expr_kind::unary_plus, expr_kind::negate));

  template <expr_kind K>
  static constexpr bool is_bop_v = boost::hana::in(
      K, boost::hana::make_tuple(
             expr_kind::plus, expr_kind::minus, expr_kind::multiplies,
             expr_kind::divides));
};

} // namespace prtcl::gt
