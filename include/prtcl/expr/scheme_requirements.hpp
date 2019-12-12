#pragma once

#include "prtcl/meta/is_any_of.hpp"
#include "prtcl/tag/kind.hpp"
#include <prtcl/data/group_base.hpp>
#include <prtcl/expr/eq.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/expr/rd.hpp>
#include <prtcl/expr/section.hpp>
#include <prtcl/expr/transform/xform_helper.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/call.hpp>
#include <prtcl/tag/group.hpp>

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/hana.hpp>
#include <boost/hana/fwd/for_each.hpp>
#include <boost/yap/algorithm.hpp>
#include <boost/yap/algorithm_fwd.hpp>
#include <boost/yap/print.hpp>

namespace prtcl::expr {

struct scheme_requirements {
  using group_predicate =
      std::function<bool(::prtcl::data::group_base const &)>;

  std::vector<any_global_field> globals;
  std::vector<std::pair<group_predicate, any_uniform_field>> uniforms;
  std::vector<std::pair<group_predicate, any_varying_field>> varyings;
};

class global_scheme_requirements_expr_xform : protected xform_helper {
public:
  template <typename TT>
  decltype(auto) operator()(term<field<tag::kind::global, TT>> term_) const {
    _reqs.globals.push_back(any_global_field{term_.value()});
    return term_;
  }

public:
  explicit global_scheme_requirements_expr_xform(scheme_requirements &reqs_)
      : _reqs{reqs_} {}

private:
  scheme_requirements &_reqs;
};

template <typename GT>
class selected_scheme_requirements_expr_xform : protected xform_helper {
  static_assert(tag::is_group_v<GT>);

  using group_predicate = scheme_requirements::group_predicate;

public:
  template <typename KT, typename TT,
            typename = std::enable_if_t<
                meta::is_any_of_v<KT, tag::kind::uniform, tag::kind::varying>>>
  decltype(auto) operator()(subs<term<field<KT, TT>>, term<GT>> subs_) const {
    if constexpr (meta::is_any_of_v<KT, tag::kind::uniform>)
      _reqs.uniforms.push_back(
          {_pred, any_uniform_field{subs_.left().value()}});
    else if constexpr (meta::is_any_of_v<KT, tag::kind::varying>)
      _reqs.varyings.push_back(
          {_pred, any_varying_field{subs_.left().value()}});
    else
      throw "invalid kind tag";
    return subs_;
  }

public:
  explicit selected_scheme_requirements_expr_xform(scheme_requirements &reqs_,
                                                   group_predicate pred_)
      : _reqs{reqs_}, _pred{pred_} {}

private:
  scheme_requirements &_reqs;
  group_predicate _pred;
};

class neighbour_loop_scheme_requirements_xform : xform_helper {
  using group_predicate = scheme_requirements::group_predicate;

public:
  template <typename F, typename... Exprs>
  void operator()(call_expr<term<selector<F>>, Exprs...> call_) {
    using namespace boost::hana::literals;
    _nl_pred = group_predicate{call_.elements[0_c].value().select};
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(call_.elements)>(
            call_.elements),
        [this](auto &&e) { boost::yap::transform_strict(e, *this); });
  }

  template <typename E> void operator()(term<eq<E>> expr_) const {
    boost::yap::transform(
        expr_.value().expression, global_scheme_requirements_expr_xform{_reqs},
        selected_scheme_requirements_expr_xform<tag::group::active>{_reqs,
                                                                    _pl_pred},
        selected_scheme_requirements_expr_xform<tag::group::passive>{
            _reqs, _nl_pred.value()});
  }

  template <typename RT, typename LHS, typename RHS>
  void operator()(term<rd<RT, LHS, RHS>> term_) const {
    // TODO: might be wrong
    boost::yap::transform(
        boost::yap::make_terminal(term_.value().lhs),
        global_scheme_requirements_expr_xform{_reqs},
        selected_scheme_requirements_expr_xform<tag::group::active>{_reqs,
                                                                    _pl_pred});
    boost::yap::transform(
        term_.value().rhs, global_scheme_requirements_expr_xform{_reqs},
        selected_scheme_requirements_expr_xform<tag::group::active>{_reqs,
                                                                    _pl_pred},
        selected_scheme_requirements_expr_xform<tag::group::passive>{
            _reqs, _nl_pred.value()});
  }

public:
  explicit neighbour_loop_scheme_requirements_xform(scheme_requirements &reqs_,
                                                    group_predicate pl_pred_)
      : _reqs{reqs_}, _pl_pred{pl_pred_} {}

private:
  scheme_requirements &_reqs;
  group_predicate _pl_pred;
  std::optional<group_predicate> _nl_pred;
};

class particle_loop_scheme_requirements_xform : xform_helper {
  using group_predicate = scheme_requirements::group_predicate;

public:
  template <typename F, typename... Exprs>
  void operator()(call_expr<term<selector<F>>, Exprs...> call_) {
    using namespace boost::hana::literals;
    _pl_pred = group_predicate{call_.elements[0_c].value().select};
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(call_.elements)>(
            call_.elements),
        [this](auto &&e) { boost::yap::transform_strict(e, *this); });
  }

  template <typename E> void operator()(term<eq<E>> expr_) const {
    boost::yap::transform(
        expr_.value().expression, global_scheme_requirements_expr_xform{_reqs},
        selected_scheme_requirements_expr_xform<tag::group::active>{
            _reqs, _pl_pred.value()});
  }

  template <typename RT, typename LHS, typename RHS>
  void operator()(term<rd<RT, LHS, RHS>> term_) const {
    // TODO: might be wrong
    boost::yap::transform(
        boost::yap::make_terminal(term_.value().lhs),
        global_scheme_requirements_expr_xform{_reqs},
        selected_scheme_requirements_expr_xform<tag::group::active>{_reqs,
                                                                    _pl_pred.value()});
    boost::yap::transform(
        term_.value().rhs, global_scheme_requirements_expr_xform{_reqs},
        selected_scheme_requirements_expr_xform<tag::group::active>{_reqs,
                                                                    _pl_pred.value()});
  }

  template <typename... Exprs>
  void operator()(call_expr<term<neighbour_loop>, Exprs...> expr) const {
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(
              e, neighbour_loop_scheme_requirements_xform{_reqs,
                                                          _pl_pred.value()});
        });
  }

public:
  explicit particle_loop_scheme_requirements_xform(scheme_requirements &reqs_)
      : _reqs{reqs_} {}

private:
  scheme_requirements &_reqs;
  std::optional<group_predicate> _pl_pred;
};

class section_scheme_requirements_xform : xform_helper {
public:
  template <typename... Exprs>
  void operator()(call_expr<term<section>, Exprs...> expr) const {
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) { boost::yap::transform_strict(e, *this); });
  }

  template <typename E> void operator()(term<eq<E>> expr_) const {
    boost::yap::transform(expr_.value().expression,
                          global_scheme_requirements_expr_xform{_reqs});
  }

  // TODO: rd<...>

  template <typename... Exprs>
  void operator()(call_expr<term<particle_loop>, Exprs...> expr) const {
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(
              e, particle_loop_scheme_requirements_xform{_reqs});
        });
  }

public:
  explicit section_scheme_requirements_xform(scheme_requirements &reqs_)
      : _reqs{reqs_} {}

private:
  scheme_requirements &_reqs;
};

template <typename Expr> auto collect_scheme_requirements(Expr &&expr_) {
  scheme_requirements reqs;
  boost::yap::transform(std::forward<Expr>(expr_),
                        section_scheme_requirements_xform{reqs});
  return std::move(reqs);
}

} // namespace prtcl::expr
