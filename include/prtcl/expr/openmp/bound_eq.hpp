#pragma once

#include <prtcl/data/openmp/scheme.hpp>
#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/eq.hpp>
#include <prtcl/expr/openmp/bind_field.hpp>
#include <prtcl/expr/openmp/call.hpp>
#include <prtcl/math/kernel/cubic_spline_kernel.hpp>
#include <prtcl/math/traits/host_math_traits.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/group.hpp>

#include <iostream>

#include <boost/hana.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::expr::openmp {

template <typename Expr> struct bound_eq {
public:
  void initialize() const {} // noop

  template <typename Ignore> void evaluate(Ignore) const {
#ifdef PRTCL_DEBUG
    std::cerr << "  prtcl::expr::openmp::bound_eq" << std::endl;
#endif
    boost::yap::evaluate(expression);
  }

  template <typename Ignore> void evaluate(Ignore, size_t pi_) const {
#ifdef PRTCL_DEBUG
    std::cerr << "          prtcl::expr::openmp::bound_eq" << std::endl;
#endif
    boost::yap::evaluate(expression, pi_);
  }

  template <typename Ignore>
  void evaluate(Ignore, size_t pi_, size_t ni_) const {
#ifdef PRTCL_DEBUG
    std::cerr << "                  prtcl::expr::openmp::bound_eq" << std::endl;
#endif
    boost::yap::evaluate(expression, pi_, ni_);
  }

  void finalize() const {} // noop

  Expr expression;
}; // namespace prtcl::expr::openmp

template <typename Expr> auto make_bound_eq(Expr &&expr_) {
  return bound_eq<meta::remove_cvref_t<Expr>>{std::forward<Expr>(expr_)};
}

struct replace_group_with_placeholder_xform : private xform_helper {
  decltype(auto) operator()(term<tag::group::active>) const {
    return boost::yap::make_terminal(boost::yap::placeholder<1>{});
  }

  decltype(auto) operator()(term<tag::group::passive>) const {
    return boost::yap::make_terminal(boost::yap::placeholder<2>{});
  }
};

template <typename Scalar, size_t N>
struct replace_call_xform : private xform_helper {
  using math_traits = ::prtcl::host_math_traits<Scalar, N>;
  using kernel_type = ::prtcl::cubic_spline_kernel<math_traits>;

  static constexpr auto call_map = make_call_map(kernel_type{});

  template <typename CT, typename = std::enable_if_t<tag::is_call_v<CT>>>
  decltype(auto) operator()(term<CT>) const {
    using boost::hana::type_c;
    return boost::yap::make_terminal(
        call_map[type_c<meta::remove_cvref_t<CT>>]);
  }
};

template <typename Scalar, size_t N> struct bind_sl_eq_xform : xform_helper {
  using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

  template <typename E> decltype(auto) operator()(term<eq<E>> term_) const {
    auto e0 = boost::yap::transform(term_.value().expression,
                                    bind_global_field_xform{_scheme});
    return make_bound_eq(
        boost::yap::transform(e0, replace_call_xform<Scalar, N>{}));
  }

public:
  bind_sl_eq_xform(scheme_type &scheme_) : _scheme{scheme_} {}

private:
  scheme_type &_scheme;
};

template <typename Scalar, size_t N> struct bind_pl_eq_xform : xform_helper {
  using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

  template <typename E> decltype(auto) operator()(term<eq<E>> term_) const {
    boost::yap::print(std::cerr, term_.value().expression);
    return make_bound_eq(boost::yap::transform(
        boost::yap::transform(
            term_.value().expression,
            bind_global_field_xform<Scalar, N>{_scheme},
            bind_uniform_field_xform<Scalar, N, tag::group::active>{
                _scheme, _particle_group_index},
            bind_varying_field_xform<Scalar, N, tag::group::active>{
                _scheme, _particle_group_index}),
        replace_group_with_placeholder_xform{},
        replace_call_xform<Scalar, N>{}));
  }

public:
  bind_pl_eq_xform(scheme_type &scheme_, size_t pgi_)
      : _scheme{scheme_}, _particle_group_index{pgi_} {}

private:
  scheme_type &_scheme;
  size_t _particle_group_index;
};

template <typename Scalar, size_t N> struct bind_nl_eq_xform : xform_helper {
  using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

  template <typename E> decltype(auto) operator()(term<eq<E>> term_) const {
    auto e0 = boost::yap::transform(
        term_.value().expression, bind_global_field_xform<Scalar, N>{_scheme},
        bind_uniform_field_xform<Scalar, N, tag::group::active>{
            _scheme, _particle_group_index},
        bind_uniform_field_xform<Scalar, N, tag::group::passive>{
            _scheme, _neighbour_group_index},
        bind_varying_field_xform<Scalar, N, tag::group::active>{
            _scheme, _particle_group_index},
        bind_varying_field_xform<Scalar, N, tag::group::passive>{
            _scheme, _neighbour_group_index});
    return make_bound_eq(
        boost::yap::transform(e0, replace_group_with_placeholder_xform{},
                              replace_call_xform<Scalar, N>{}));
  }

public:
  bind_nl_eq_xform(scheme_type &scheme_, size_t pgi_, size_t ngi_)
      : _scheme{scheme_}, _particle_group_index{pgi_}, _neighbour_group_index{
                                                           ngi_} {}

private:
  scheme_type &_scheme;
  size_t _particle_group_index;
  size_t _neighbour_group_index;
};

} // namespace prtcl::expr::openmp
