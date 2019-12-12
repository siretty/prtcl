#pragma once

#include <boost/hana/fwd/for_each.hpp>
#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/eq.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/expr/openmp/bind_field.hpp>
#include <prtcl/expr/openmp/bound_eq.hpp>
#include <prtcl/expr/openmp/prepared_loop.hpp>
#include <prtcl/expr/rd.hpp>
#include <prtcl/expr/section.hpp>
#include <prtcl/expr/transform/xform_helper.hpp>
#include <prtcl/meta/remove_cvref.hpp>

#include <algorithm>
#include <iostream>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include <boost/hana/fwd/integral_constant.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::expr::openmp {

template <typename Tuple> struct bound_section {
  void initialize() const {}

  template <typename Grid> void evaluate(Grid &grid_) const {
#ifdef PRTCL_DEBUG // {{{
    std::cerr << "prtcl::expr::openmp::bound_section" << std::endl;
#endif // }}}
    boost::hana::for_each(parts, [&grid_](auto &&part) {
      std::forward<decltype(part)>(part).evaluate(grid_);
    });
  }

  void finalize() const {}

  std::string name;
  Tuple parts;
};

template <typename Scheme, typename Tuple> struct bound_particle_loop {
  void initialize() const {}

  template <typename Grid> void evaluate(Grid &grid_) const {
    std::vector<std::vector<size_t>> neighbours;
#ifdef PRTCL_DEBUG // {{{
    std::cerr << "  prtcl::expr::openmp::bound_particle_loop" << std::endl;
#endif // }}}
    for (auto &part : parts) {
#ifdef PRTCL_DEBUG // {{{
      std::cerr << "    particle group index: " << pgi << std::endl;
#endif // }}}
       //#pragma omp parallel for private(neighbours)
      for (size_t pi = 0; pi < scheme.get_group(part.first).size(); ++pi) {
        // find neighbours
        grid_.neighbours(
            part.first, pi, scheme,
            [&neighbours /*, &total_neighbours*/](auto gi, auto ri) {
              neighbours[gi].push_back(ri);
              //#pragma omp atomic
              //                total_neighbours += 1;
            });
#ifdef PRTCL_DEBUG // {{{
        std::cerr << "      particle index: " << pi << std::endl;
#endif // }}}
        boost::hana::for_each(part.second, [&neighbours, pi](auto &&opt_sel) {
          if (std::forward<decltype(opt_sel)>(opt_sel)) {
#ifdef PRTCL_DEBUG // {{{
            std::cerr << "        selected" << std::endl;
#endif // }}}
            boost::hana::for_each(
                std::forward<decltype(opt_sel)>(opt_sel).value(),
                [&neighbours, pi](auto &&part) {
                  std::forward<decltype(part)>(part).evaluate(neighbours, pi);
                });
          } else {
#ifdef PRTCL_DEBUG // {{{
            std::cerr << "        not selected" << std::endl;
#endif // }}}
          }
        });
      }
    }
  }

  void finalize() const {}

  Scheme &scheme;
  std::vector<std::pair<size_t, Tuple>> parts;
};

template <typename Scheme, typename Tuple> struct bound_neighbour_loop {
  void initialize() const {}

  template <typename Neighbours>
  void evaluate(Neighbours &neighbours, size_t pi_) const {
#ifdef PRTCL_DEBUG // {{{
    std::cerr << "          prtcl::expr::openmp::bound_neighbour_loop"
              << std::endl;
#endif // }}}
    for (auto const &[ngi, tuple] : parts) {
#ifdef PRTCL_DEBUG // {{{
      std::cerr << "            neighbour group index: " << ngi << std::endl;
#endif // }}}
      // for (size_t ni = 0; ni < scheme.get_group(ngi).size(); ++ni) {
      for (size_t ni : neighbours[ngi]) {
#ifdef PRTCL_DEBUG // {{{
        std::cerr << "              neighbour index: " << ni << std::endl;
#endif // }}}
        boost::hana::for_each(
            tuple, [&neighbours, pi = pi_, ni = ni](auto &&opt_sel) {
              if (std::forward<decltype(opt_sel)>(opt_sel)) {
#ifdef PRTCL_DEBUG // {{{
                std::cerr << "                selected" << std::endl;
#endif // }}}
                boost::hana::for_each(
                    std::forward<decltype(opt_sel)>(opt_sel).value(),
                    [&neighbours, pi, ni](auto &&part) {
                      std::forward<decltype(part)>(part).evaluate(neighbours,
                                                                  pi, ni);
                    });
              } else {
#ifdef PRTCL_DEBUG // {{{
                std::cerr << "                not selected" << std::endl;
#endif // }}}
              }
            });
      }
    }
  };

  void finalize() const {}

  Scheme &scheme;
  std::vector<std::pair<size_t, Tuple>> parts;
};

template <typename Scalar, size_t N>
struct bind_prepared_neighbour_loop_selector_xform : private xform_helper {
  using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

private:
  template <typename Expr> decltype(auto) _transform_one(Expr &&expr_) const {
    return boost::yap::transform_strict(
        std::forward<Expr>(expr_),
        bind_nl_eq_xform<Scalar, N>{_scheme, _particle_group_index,
                                    _neighbour_group_index}
        // TODO: , bind_nl_rd_xform<...>{...}
    );
  }

  template <size_t SliceTo, typename Tuple>
  decltype(auto) _transform_all(Tuple &&tuple_) const {
    return boost::hana::transform(
        boost::hana::slice_c<1, SliceTo>(std::forward<Tuple>(tuple_)),
        [this](auto &&expr_) -> decltype(auto) {
          return this->_transform_one(std::forward<decltype(expr_)>(expr_));
        });
  }

public:
  template <typename... Es>
  decltype(auto)
  operator()(call_expr<term<prepared_selector>, Es...> call_) const {
    using namespace boost::hana::literals;
    auto const &sel = call_.elements[0_c].value();
    constexpr size_t SliceTo = sizeof...(Es) + 1;
    using result_type =
        std::optional<decltype(_transform_all<SliceTo>(call_.elements))>;
    if (std::binary_search(sel.groups.begin(), sel.groups.end(),
                           _neighbour_group_index))
      return result_type{_transform_all<SliceTo>(call_.elements)};
    else
      return result_type{};
  }

public:
  explicit bind_prepared_neighbour_loop_selector_xform(scheme_type &scheme_,
                                                       size_t pgi_, size_t ngi_)
      : _scheme{scheme_}, _particle_group_index{pgi_}, _neighbour_group_index{
                                                           ngi_} {}

private:
  scheme_type &_scheme;
  size_t _particle_group_index;
  size_t _neighbour_group_index;
};

template <typename Scalar, size_t N>
struct bind_prepared_neighbour_loop_xform : private xform_helper {
  using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

private:
  template <typename Expr>
  decltype(auto) _transform_one(size_t ngi_, Expr &&expr_) const {
    return boost::yap::transform_strict(
        std::forward<Expr>(expr_),
        bind_prepared_neighbour_loop_selector_xform<Scalar, N>{
            _scheme, _particle_group_index, ngi_});
  }

  template <size_t SliceTo, typename Tuple>
  decltype(auto) _transform_all(size_t ngi_, Tuple &&tuple_) const {
    return boost::hana::transform(
        boost::hana::slice_c<1, SliceTo>(std::forward<Tuple>(tuple_)),
        [this, ngi_](auto &&expr_) -> decltype(auto) {
          return this->_transform_one(ngi_,
                                      std::forward<decltype(expr_)>(expr_));
        });
  }

public:
  template <typename... Es>
  decltype(auto)
  operator()(call_expr<term<prepared_neighbour_loop>, Es...> call_) const {
    using namespace boost::hana::literals;
    auto const &loop = call_.elements[0_c].value();
    constexpr size_t SliceTo = sizeof...(Es) + 1;
    using result_type = decltype(_transform_all<SliceTo>(0UL, call_.elements));
    bound_neighbour_loop<scheme_type, result_type> result{_scheme, {}};
    for (auto ngi : loop.groups)
      result.parts.emplace_back(ngi,
                                _transform_all<SliceTo>(ngi, call_.elements));
    return result;
  }

public:
  bind_prepared_neighbour_loop_xform(scheme_type &scheme_, size_t pgi_)
      : _scheme{scheme_}, _particle_group_index{pgi_} {}

private:
  scheme_type &_scheme;
  size_t _particle_group_index;
};

template <typename Scalar, size_t N>
struct bind_prepared_particle_loop_selector_xform : private xform_helper {
  using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

private:
  template <typename Expr> decltype(auto) _transform_one(Expr &&expr_) const {
    return boost::yap::transform_strict(
        std::forward<Expr>(expr_),
        bind_prepared_neighbour_loop_xform<Scalar, N>{_scheme,
                                                      _particle_group_index},
        bind_pl_eq_xform<Scalar, N>{_scheme, _particle_group_index}
        // TODO: , bind_nl_rd_xform<...>{...}
    );
  }

  template <size_t SliceTo, typename Tuple>
  decltype(auto) _transform_all(Tuple &&tuple_) const {
    return boost::hana::transform(
        boost::hana::slice_c<1, SliceTo>(std::forward<Tuple>(tuple_)),
        [this](auto &&expr_) -> decltype(auto) {
          return this->_transform_one(std::forward<decltype(expr_)>(expr_));
        });
  }

public:
  template <typename... Es>
  decltype(auto)
  operator()(call_expr<term<prepared_selector>, Es...> call_) const {
    using namespace boost::hana::literals;
    auto const &sel = call_.elements[0_c].value();
    constexpr size_t SliceTo = sizeof...(Es) + 1;
    using result_type =
        std::optional<decltype(_transform_all<SliceTo>(call_.elements))>;
    if (std::binary_search(sel.groups.begin(), sel.groups.end(),
                           _particle_group_index))
      return result_type{_transform_all<SliceTo>(call_.elements)};
    else
      return result_type{};
  }

public:
  explicit bind_prepared_particle_loop_selector_xform(scheme_type &scheme_,
                                                      size_t pgi_)
      : _scheme{scheme_}, _particle_group_index{pgi_} {}

private:
  scheme_type &_scheme;
  size_t _particle_group_index;
};

template <typename Scalar, size_t N>
struct bind_prepared_particle_loop_xform : private xform_helper {
  using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

private:
  template <typename Expr>
  decltype(auto) _transform_one(size_t pgi_, Expr &&expr_) const {
    return boost::yap::transform_strict(
        std::forward<Expr>(expr_),
        bind_prepared_particle_loop_selector_xform<Scalar, N>{_scheme, pgi_});
  }

  template <size_t SliceTo, typename Tuple>
  decltype(auto) _transform_all(size_t pgi_, Tuple &&tuple_) const {
    return boost::hana::transform(
        boost::hana::slice_c<1, SliceTo>(std::forward<Tuple>(tuple_)),
        [this, pgi_](auto &&expr_) -> decltype(auto) {
          return this->_transform_one(pgi_,
                                      std::forward<decltype(expr_)>(expr_));
        });
  }

public:
  template <typename... Es>
  decltype(auto)
  operator()(call_expr<term<prepared_particle_loop>, Es...> call_) const {
    using namespace boost::hana::literals;
    auto const &loop = call_.elements[0_c].value();
    constexpr size_t SliceTo = sizeof...(Es) + 1;
    using result_type = decltype(_transform_all<SliceTo>(0UL, call_.elements));
    bound_particle_loop<scheme_type, result_type> result{_scheme, {}};
    for (auto pgi : loop.groups)
      result.parts.emplace_back(pgi,
                                _transform_all<SliceTo>(pgi, call_.elements));
    return result;
  }

public:
  explicit bind_prepared_particle_loop_xform(scheme_type &scheme_)
      : _scheme{scheme_} {}

private:
  scheme_type &_scheme;
};

template <typename Scalar, size_t N>
struct bind_section_xform : private xform_helper {
  using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

private:
  template <typename Expr> decltype(auto) _transform_one(Expr &&expr_) const {
    return boost::yap::transform_strict(
        std::forward<Expr>(expr_),
        bind_prepared_particle_loop_xform<Scalar, N>{_scheme},
        bind_sl_eq_xform<Scalar, N>{_scheme});
  }

  template <size_t SliceTo, typename Tuple>
  decltype(auto) _transform_all(Tuple &&tuple_) const {
    return boost::hana::transform(
        boost::hana::slice_c<1, SliceTo>(std::forward<Tuple>(tuple_)),
        [this](auto &&expr_) -> decltype(auto) {
          return this->_transform_one(std::forward<decltype(expr_)>(expr_));
        });
  }

public:
  template <typename... Es>
  decltype(auto) operator()(call_expr<term<section>, Es...> call_) const {
    using namespace boost::hana::literals;
    auto const &section = call_.elements[0_c].value();
    constexpr size_t SliceTo = sizeof...(Es) + 1;
    using result_type = decltype(_transform_all<SliceTo>(call_.elements));
    return bound_section<result_type>{section.name,
                                      _transform_all<SliceTo>(call_.elements)};
  }

public:
  explicit bind_section_xform(scheme_type &scheme_) : _scheme{scheme_} {}

private:
  scheme_type &_scheme;
};

} // namespace prtcl::expr::openmp
