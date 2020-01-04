#pragma once

#include <prtcl/expr/field.hpp>
#include <prtcl/expr/transform/group_tag_terminal_value_xform.hpp>
#include <prtcl/expr/transform/xform_helper.hpp>
#include <prtcl/meta/format_cxx_type.hpp>
#include <prtcl/meta/is_any_of.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/group.hpp>
#include <prtcl/tag/kind.hpp>
#include <prtcl/tag/reduce.hpp>

#include <iostream>

#include <boost/hana.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename RT, typename LHS, typename RHS> struct rd {
  static_assert(tag::is_reduce_v<RT>);
  static_assert(::boost::yap::is_expr<LHS>::value);
  static_assert(::boost::yap::is_expr<RHS>::value);

  using reduce_tag_type = RT;
  static constexpr reduce_tag_type reduce_tag = {};

  LHS lhs;
  RHS rhs;
};

template <typename RT, typename LHS, typename RHS>
auto make_rd(LHS &&lhs, RHS &&rhs) {
  return rd<RT, meta::remove_cvref_t<LHS>, meta::remove_cvref_t<RHS>>{
      std::forward<LHS>(lhs), std::forward<RHS>(rhs)};
}

template <typename F> class rd_xform_base {
public:
  template <typename U>
  explicit rd_xform_base(U &&callable_)
      : _callable{std::forward<U>(callable_)} {}

protected:
  template <typename RT, typename LHS, typename RHS>
  decltype(auto) rd_transform(RT &&rt, LHS &&lhs, RHS &&rhs) {
    static_assert(tag::is_reduce_v<meta::remove_cvref_t<RT>>);
    return _callable(
        std::forward<RT>(rt), std::forward<LHS>(lhs), std::forward<RHS>(rhs));
  }

  template <typename RT, typename LHS, typename RHS>
  decltype(auto) rd_transform(RT &&rt, LHS &&lhs, RHS &&rhs) const {
    static_assert(tag::is_reduce_v<meta::remove_cvref_t<RT>>);
    return _callable(
        std::forward<RT>(rt), std::forward<LHS>(lhs), std::forward<RHS>(rhs));
  }

private:
  F _callable;
};

template <template <typename> typename T, typename F>
auto make_rd_xform(F &&callable_) {
  return T<meta::remove_cvref_t<F>>{std::forward<F>(callable_)};
}

template <template <typename> typename T> auto make_rd_xform() {
  return make_rd_xform<T>(
      [](auto &&e) -> auto { return std::forward<decltype(e)>(e); });
}

// struct make_reduce_tag<...> {{{

template <boost::yap::expr_kind> struct make_reduce_tag;

template <> struct make_reduce_tag<boost::yap::expr_kind::plus_assign> {
  using type = tag::reduce::plus;
};

template <> struct make_reduce_tag<boost::yap::expr_kind::minus_assign> {
  using type = tag::reduce::minus;
};

template <> struct make_reduce_tag<boost::yap::expr_kind::multiplies_assign> {
  using type = tag::reduce::multiplies;
};

template <> struct make_reduce_tag<boost::yap::expr_kind::divides_assign> {
  using type = tag::reduce::divides;
};

template <boost::yap::expr_kind K>
using make_reduce_tag_t = typename make_reduce_tag<K>::type;

template <boost::yap::expr_kind K>
constexpr make_reduce_tag_t<K> make_reduce_tag_v = {};

// }}}

template <typename F>
struct grd_xform : private xform_helper, public rd_xform_base<F> {
  template <
      expr_kind K, typename TT, typename RHS,
      typename = std::enable_if_t<is_opassign_v<K>>>
  decltype(auto)
  operator()(expr<K, term<field<tag::kind::global, TT>>, RHS> expr) const {
    return this->rd_transform(make_reduce_tag_v<K>, expr.left(), expr.right());
  }

  template <
      typename TT, typename RT, typename RHS,
      typename = std::enable_if_t<tag::is_reduce_v<RT>>>
  decltype(auto)
  operator()(expr<
             expr_kind::assign, term<field<tag::kind::global, TT>>,
             call_expr<term<RT>, RHS>>
                 expr) const {
    using namespace boost::hana::literals;
    return this->rd_transform(
        meta::remove_cvref_t<RT>{}, expr.left(), expr.right().elements[1_c]);
  }

  using rd_xform_base<F>::rd_xform_base;
};

template <typename F>
struct urd_xform : private xform_helper, public rd_xform_base<F> {
  template <
      expr_kind K, typename TT, typename RHS,
      typename = std::enable_if_t<is_opassign_v<K>>>
  decltype(auto) operator()(
      expr<
          K,
          subs<term<field<tag::kind::uniform, TT>>, term<tag::group::active>>,
          RHS>
          expr) const {
    return this->rd_transform(
        make_reduce_tag_v<K>, expr.left().left(), expr.right());
  }

  template <
      typename TT, typename RT, typename RHS,
      typename = std::enable_if_t<tag::is_reduce_v<RT>>>
  decltype(auto) operator()(
      expr<
          expr_kind::assign,
          subs<term<field<tag::kind::uniform, TT>>, term<tag::group::active>>,
          call_expr<term<RT>, RHS>>
          expr) const {
    using namespace boost::hana::literals;
    return this->rd_transform(
        meta::remove_cvref_t<RT>{}, expr.left().left(),
        expr.right().elements[1_c]);
  }

  using rd_xform_base<F>::rd_xform_base;
};

template <typename Expr> auto _grd(Expr &&e0) {
  auto e1 = boost::yap::transform(
      std::forward<Expr>(e0), group_tag_terminal_value_xform{});
  return boost::yap::transform_strict(
      e1, make_rd_xform<grd_xform>([](auto, auto, auto) { return true; }),
      [](auto) { return false; });
}

template <typename Expr> auto _urd(Expr &&e0) {
  auto e1 = boost::yap::transform(
      std::forward<Expr>(e0), group_tag_terminal_value_xform{});
  return boost::yap::transform_strict(
      e1, make_rd_xform<urd_xform>([](auto, auto, auto) { return true; }),
      [](auto) { return false; });
}

} // namespace prtcl::expr

namespace prtcl::expr_language {

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::reduce::max>>
    reduce_max = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::reduce::min>>
    reduce_min = {};

} // namespace prtcl::expr_language
