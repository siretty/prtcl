#pragma once

#include <prtcl/expr/eq.hpp>
#include <prtcl/expr/rd.hpp>
#include <prtcl/expr/transform/xform_helper.hpp>
#include <prtcl/meta/remove_cvref.hpp>

#include <utility>

#include <boost/hana/fwd/integral_constant.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::expr {

struct particle_loop {};
struct neighbour_loop {};

template <typename F> struct selector { F select; };

struct foreach_particle_identity_xform : private xform_helper {
  template <typename... Es>
  decltype(auto) operator()(call_expr<term<particle_loop>, Es...> e) const {
    return e;
  }
};

struct foreach_neighbour_identity_xform : private xform_helper {
  template <typename... Es>
  decltype(auto) operator()(call_expr<term<neighbour_loop>, Es...> e) const {
    return e;
  }
};

struct foreach_particle_selector_identity_xform : private xform_helper {
  template <typename F, typename... Es>
  decltype(auto)
  operator()(call_expr<term<selector<F>>, Es...> sel_expr) const {
    /*
    std::cout << "# ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ #"
              << "\n";
    display_cxx_type(sel_expr, std::cout);
    std::cout << "# ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ #"
              << "\n";
    */
    using namespace boost::hana::literals;
    return boost::hana::unpack(
        boost::hana::concat(
            boost::hana::make_tuple(sel_expr.elements[0_c]),
            boost::hana::transform(
                boost::hana::slice_c<1, sizeof...(Es) + 1>(sel_expr.elements),
                [](auto &&sexpr) {
                  return boost::yap::transform_strict(
                      std::forward<decltype(sexpr)>(sexpr),
                      make_eq_xform<veq_xform>([](auto &&e) {
                        return boost::yap::make_terminal(
                            make_eq(std::forward<decltype(e)>(e)));
                      }),
                      make_rd_xform<grd_xform>(
                          [](auto rt, auto &&lhs, auto &&rhs) {
                            return boost::yap::make_terminal(
                                make_rd<meta::remove_cvref_t<decltype(rt)>>(
                                    std::forward<decltype(lhs)>(lhs),
                                    std::forward<decltype(rhs)>(rhs)));
                          }),
                      make_rd_xform<urd_xform>(
                          [](auto rt, auto &&lhs, auto &&rhs) {
                            return boost::yap::make_terminal(
                                make_rd<meta::remove_cvref_t<decltype(rt)>>(
                                    std::forward<decltype(lhs)>(lhs),
                                    std::forward<decltype(rhs)>(rhs)));
                          }),
                      foreach_neighbour_identity_xform{});
                })),
        [](auto &&... args) {
          return boost::yap::make_expression<expr_kind::call>(
              std::forward<decltype(args)>(args)...);
        });
    /*
    boost::hana::for_each(
        boost::hana::slice_c<1, sizeof...(Es) + 1>(sel_expr.elements),
        [](auto &&e) {
          boost::yap::transform_strict(std::forward<decltype(e)>(e),
                                       make_eq_xform<veq_xform>(),
                                       foreach_neighbour_identity_xform{});
        });
    return sel_expr;
    */
  }
};

struct foreach_neighbour_selector_identity_xform : private xform_helper {
  template <typename F, typename... Es>
  decltype(auto)
  operator()(call_expr<term<selector<F>>, Es...> sel_expr) const {
    /*
    std::cout << "# ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ ∨ #"
              << "\n";
    display_cxx_type(sel_expr, std::cout);
    std::cout << "# ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ ∧ #"
              << "\n";
    */
    using namespace boost::hana::literals;
    return boost::hana::unpack(
        boost::hana::concat(
            boost::hana::make_tuple(sel_expr.elements[0_c]),
            boost::hana::transform(
                boost::hana::slice_c<1, sizeof...(Es) + 1>(sel_expr.elements),
                [](auto &&sexpr) {
                  return boost::yap::transform_strict(
                      std::forward<decltype(sexpr)>(sexpr),
                      make_eq_xform<veq_xform>([](auto &&e) {
                        return boost::yap::make_terminal(
                            make_eq(std::forward<decltype(e)>(e)));
                      }),
                      make_rd_xform<grd_xform>(
                          [](auto rt, auto &&lhs, auto &&rhs) {
                            return boost::yap::make_terminal(
                                make_rd<meta::remove_cvref_t<decltype(rt)>>(
                                    std::forward<decltype(lhs)>(lhs),
                                    std::forward<decltype(rhs)>(rhs)));
                          }),
                      make_rd_xform<urd_xform>(
                          [](auto rt, auto &&lhs, auto &&rhs) {
                            return boost::yap::make_terminal(
                                make_rd<meta::remove_cvref_t<decltype(rt)>>(
                                    std::forward<decltype(lhs)>(lhs),
                                    std::forward<decltype(rhs)>(rhs)));
                          }));
                })),
        [](auto &&... args) {
          return boost::yap::make_expression<expr_kind::call>(
              std::forward<decltype(args)>(args)...);
        });
    /*
    boost::hana::for_each(
      boost::hana::slice_c<1, sizeof...(Es) + 1>(sel_expr.elements),
      [](auto &&sexpr) {
        boost::yap::transform_strict(
            std::forward<decltype(sexpr)>(sexpr),
            make_eq_xform<veq_xform>([](auto &&e) {
              return boost::yap::make_terminal(
                  make_eq(std::forward<decltype(e)>(e)));
            }));
      });
    return sel_expr;
    */
  }
};

template <typename Select> auto only(Select &&select_) {
  using selector_type = selector<meta::remove_cvref_t<Select>>;
  return boost::yap::expression<boost::yap::expr_kind::terminal,
                                boost::hana::tuple<selector_type>>{
      selector_type{std::forward<Select>(select_)}};
}

template <typename... SExprs> auto foreach_particle(SExprs &&... sexprs_) {
  return boost::yap::make_terminal(particle_loop{})(
      boost::yap::transform_strict(
          boost::yap::transform(std::forward<SExprs>(sexprs_),
                                group_tag_terminal_value_xform{}),
          foreach_particle_selector_identity_xform{})...);
}

template <typename... SExprs> auto foreach_neighbour(SExprs &&... sexprs_) {
  return boost::yap::make_terminal(neighbour_loop{})(
      boost::yap::transform_strict(
          boost::yap::transform(std::forward<SExprs>(sexprs_),
                                group_tag_terminal_value_xform{}),
          foreach_neighbour_selector_identity_xform{})...);
}

} // namespace prtcl::expr

namespace prtcl::expr_language {

using ::prtcl::expr::foreach_neighbour;
using ::prtcl::expr::foreach_particle;
using ::prtcl::expr::only;

} // namespace prtcl::expr_language
