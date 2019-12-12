#pragma once

#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/eq.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/expr/rd.hpp>
#include <prtcl/expr/transform/xform_helper.hpp>
#include <prtcl/meta/remove_cvref.hpp>

#include <set>
#include <utility>
#include <vector>

#include <boost/hana/fwd/integral_constant.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::expr::openmp {

struct prepared_particle_loop {
  std::vector<size_t> groups;
};

struct prepared_neighbour_loop {
  std::vector<size_t> groups;
};

struct prepared_selector {
  std::vector<size_t> groups;
};

template <typename Scalar, size_t N, typename F>
auto make_prepared_selector(data::scheme<Scalar, N> &scheme_,
                            expr::selector<F> &sel_) {
  std::vector<size_t> groups;
  for (size_t gi = 0; gi < scheme_.get_group_count(); ++gi) {
    auto &g = scheme_.get_group(gi);
    if (sel_.select(g))
      groups.push_back(gi);
  }
  return prepared_selector{groups};
}

struct eq_rd_identity_xform : private xform_helper {
  template <typename E>
  decltype(auto) operator()(term<prtcl::expr::eq<E>> term_) const {
    return term_;
  }

  template <typename RT, typename LHS, typename RHS>
  decltype(auto) operator()(term<prtcl::expr::rd<RT, LHS, RHS>> term_) const {
    return term_;
  }
};

template <typename Scalar, size_t N>
struct prepared_neighbour_loop_selector_xform : private xform_helper {
public:
  using scheme_type = ::prtcl::data::scheme<Scalar, N>;

public:
  template <typename F, typename... Es>
  decltype(auto)
  operator()(call_expr<term<prtcl::expr::selector<F>>, Es...> call_) const {
    using namespace boost::hana::literals;
    return boost::hana::unpack(
        boost::hana::concat(
            boost::hana::make_tuple(
                make_prepared_selector(_scheme, call_.elements[0_c].value())),
            boost::hana::transform(
                boost::hana::slice_c<1, sizeof...(Es) + 1>(call_.elements),
                [](auto &&e) {
                  return boost::yap::transform_strict(
                      std::forward<decltype(e)>(e), eq_rd_identity_xform{});
                })),
        [](auto &&... args) {
          return boost::yap::make_expression<expr_kind::call>(
              std::forward<decltype(args)>(args)...);
        });
  }

public:
  prepared_neighbour_loop_selector_xform(scheme_type &scheme_)
      : _scheme{scheme_} {}

private:
  scheme_type &_scheme;
};

template <typename Scalar, size_t N>
struct prepared_neighbour_loop_xform : private xform_helper {
public:
  using scheme_type = ::prtcl::data::scheme<Scalar, N>;

public:
  template <typename... Es>
  decltype(auto)
  operator()(call_expr<term<prtcl::expr::neighbour_loop>, Es...> call_) const {
    using namespace boost::hana::literals;
    std::set<size_t> groups;
    auto selectors = boost::hana::transform(
        boost::hana::slice_c<1, sizeof...(Es) + 1>(call_.elements),
        [this, &groups](auto &&sexpr) {
          // transform the selector call-exprs
          auto sel = boost::yap::transform_strict(
              std::forward<decltype(sexpr)>(sexpr),
              prepared_neighbour_loop_selector_xform<Scalar, N>{_scheme});
          // collect all used groups in this loop
          for (size_t gi : sel.elements[0_c].value().groups)
            groups.insert(gi);
          return sel;
        });
    // return a new call-expr
    return boost::hana::unpack(
        boost::hana::concat(boost::hana::make_tuple(prepared_neighbour_loop{
                                {groups.begin(), groups.end()}}),
                            selectors),
        [](auto &&... args) {
          return boost::yap::make_expression<expr_kind::call>(
              std::forward<decltype(args)>(args)...);
        });
  }

public:
  prepared_neighbour_loop_xform(scheme_type &scheme_) : _scheme{scheme_} {}

private:
  scheme_type &_scheme;
};

template <typename Scalar, size_t N>
struct prepared_particle_loop_selector_xform : private xform_helper {
public:
  using scheme_type = ::prtcl::data::scheme<Scalar, N>;

public:
  template <typename F, typename... Es>
  decltype(auto)
  operator()(call_expr<term<prtcl::expr::selector<F>>, Es...> call_) const {
    using namespace boost::hana::literals;
    return boost::hana::unpack(
        boost::hana::concat(
            boost::hana::make_tuple(
                make_prepared_selector(_scheme, call_.elements[0_c].value())),
            boost::hana::transform(
                boost::hana::slice_c<1, sizeof...(Es) + 1>(call_.elements),
                [this](auto &&e) {
                  return boost::yap::transform_strict(
                      std::forward<decltype(e)>(e), eq_rd_identity_xform{},
                      prepared_neighbour_loop_xform<Scalar, N>{_scheme});
                })),
        [](auto &&... args) {
          return boost::yap::make_expression<expr_kind::call>(
              std::forward<decltype(args)>(args)...);
        });
  }

public:
  prepared_particle_loop_selector_xform(scheme_type &scheme_)
      : _scheme{scheme_} {}

private:
  scheme_type &_scheme;
};

template <typename Scalar, size_t N>
class prepared_particle_loop_xform : private xform_helper {
public:
  using scheme_type = ::prtcl::data::scheme<Scalar, N>;

public:
  template <typename... Es>
  decltype(auto)
  operator()(call_expr<term<prtcl::expr::particle_loop>, Es...> call_) const {
    using namespace boost::hana::literals;
    std::set<size_t> groups;
    auto selectors = boost::hana::transform(
        boost::hana::slice_c<1, sizeof...(Es) + 1>(call_.elements),
        [this, &groups](auto &&s) {
          // transform the selector call-exprs
          auto sel = boost::yap::transform_strict(
              std::forward<decltype(s)>(s),
              prepared_particle_loop_selector_xform<Scalar, N>{_scheme});
          // collect all used groups in this loop
          for (size_t gi : sel.elements[0_c].value().groups)
            groups.insert(gi);
          return sel;
        });
    // return a new call-expr
    return boost::hana::unpack(
        boost::hana::concat(boost::hana::make_tuple(prepared_particle_loop{
                                {groups.begin(), groups.end()}}),
                            selectors),
        [](auto &&... args) {
          return boost::yap::make_expression<expr_kind::call>(
              std::forward<decltype(args)>(args)...);
        });
  }

public:
  prepared_particle_loop_xform(scheme_type &scheme_) : _scheme{scheme_} {}

private:
  scheme_type &_scheme;
};

} // namespace prtcl::expr::openmp
