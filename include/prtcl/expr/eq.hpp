#pragma once

#include "prtcl/meta/is_any_of.hpp"
#include "prtcl/meta/remove_cvref.hpp"
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/transform/group_tag_terminal_value_xform.hpp>
#include <prtcl/expr/transform/xform_helper.hpp>
#include <prtcl/meta/format_cxx_type.hpp>
#include <prtcl/tag/group.hpp>
#include <prtcl/tag/kind.hpp>

#include <iostream>

#include <boost/hana.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename E> struct eq { E expression; };

template <typename E> auto make_eq(E &&e) {
  return eq<meta::remove_cvref_t<E>>{e};
}

template <typename F> class eq_xform_base {
public:
  template <typename U>
  explicit eq_xform_base(U &&callable_)
      : _callable{std::forward<U>(callable_)} {}

protected:
  template <typename E> decltype(auto) call(E &&e) {
    return _callable(std::forward<E>(e));
  }

  template <typename E> decltype(auto) call(E &&e) const {
    return _callable(std::forward<E>(e));
  }

private:
  F _callable;
};

template <template <typename> typename T, typename F>
auto make_eq_xform(F &&callable_) {
  return T<meta::remove_cvref_t<F>>{std::forward<F>(callable_)};
}

template <template <typename> typename T> auto make_eq_xform() {
  return make_eq_xform<T>(
      [](auto &&e) -> auto { return std::forward<decltype(e)>(e); });
}

template <typename F>
struct geq_xform : private xform_helper, public eq_xform_base<F> {
  template <expr_kind K, typename TT, typename RHS,
            typename = std::enable_if_t<is_assign_v<K> or is_opassign_v<K>>>
  decltype(auto)
  operator()(expr<K, term<field<tag::kind::global, TT>>, RHS> expr) const {
    return this->call(expr);
  }

  using eq_xform_base<F>::eq_xform_base;
};

template <typename F>
struct ueq_xform : private xform_helper, public eq_xform_base<F> {
  template <expr_kind K, typename TT, typename RHS,
            typename = std::enable_if_t<is_assign_v<K> or is_opassign_v<K>>>
  decltype(auto) operator()(
      expr<K,
           subs<term<field<tag::kind::uniform, TT>>, term<tag::group::active>>,
           RHS>
          expr) const {
    return this->call(expr);
  }

  using eq_xform_base<F>::eq_xform_base;
};

template <typename F>
struct veq_xform : private xform_helper, public eq_xform_base<F> {
  template <expr_kind K, typename TT, typename RHS,
            typename = std::enable_if_t<is_assign_v<K> or is_opassign_v<K>>>
  decltype(auto) operator()(
      expr<K,
           subs<term<field<tag::kind::varying, TT>>, term<tag::group::active>>,
           RHS>
          expr) const {
    return this->call(expr);
  }

  using eq_xform_base<F>::eq_xform_base;
};

template <typename Expr> auto _geq(Expr &&e0) {
  auto e1 = boost::yap::transform(std::forward<Expr>(e0),
                                  group_tag_terminal_value_xform{});
  return boost::yap::transform_strict(
      e1, make_eq_xform<geq_xform>([](auto) { return true; }),
      [](auto) { return false; });
}

template <typename Expr> auto _ueq(Expr &&e0) {
  auto e1 = boost::yap::transform(std::forward<Expr>(e0),
                                  group_tag_terminal_value_xform{});
  return boost::yap::transform_strict(
      e1, make_eq_xform<ueq_xform>([](auto) { return true; }),
      [](auto) { return false; });
}

template <typename Expr> auto _veq(Expr &&e0) {
  auto e1 = boost::yap::transform(std::forward<Expr>(e0),
                                  group_tag_terminal_value_xform{});
  return boost::yap::transform_strict(
      e1, make_eq_xform<veq_xform>([](auto) { return true; }),
      [](auto) { return false; });
}

} // namespace prtcl::expr
