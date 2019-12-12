#pragma once

#include "prtcl/math/kernel/cubic_spline_kernel.hpp"
#include "prtcl/math/traits/host_math_traits.hpp"
#include "prtcl/meta/remove_cvref.hpp"
#include <prtcl/data/host/compact_uniform_grid.hpp>
#include <prtcl/data/host/grouped_uniform_grid.hpp>
#include <prtcl/data/openmp/scheme.hpp>
#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/openmp/bound_section.hpp>
#include <prtcl/expr/openmp/prepared_loop.hpp>
#include <prtcl/expr/scheme_requirements.hpp>

#include <cstddef>
#include <memory>
#include <utility>

#include <boost/hana.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::scheme {

/*
  auto e0 = boost::yap::transform(
      sesph_density_pressure,
      prtcl::expr::openmp::prepared_particle_loop_xform<float, 3>{scheme});
  auto e1 = boost::yap::transform(
      e0, prtcl::expr::openmp::bind_section_xform<float, 3>{omp_scheme});
*/

template <typename Sections, typename Scalar, size_t N>
auto openmp_prepare(Sections &&sections_,
                    ::prtcl::data::scheme<Scalar, N> &scheme_) {
  return boost::hana::transform(
      std::forward<Sections>(sections_), [&scheme_](auto &&section_) {
        return boost::yap::transform(
            std::forward<decltype(section_)>(section_),
            prtcl::expr::openmp::prepared_particle_loop_xform<Scalar, N>{
                scheme_});
      });
}

template <typename PreparedSections, typename Scalar, size_t N>
auto openmp_bind(PreparedSections &&prepared_sections_,
                 ::prtcl::data::openmp::scheme<Scalar, N> &scheme_) {
  return boost::hana::transform(
      std::forward<PreparedSections>(prepared_sections_),
      [&scheme_](auto &&section_) {
        return boost::yap::transform(
            std::forward<decltype(section_)>(section_),
            prtcl::expr::openmp::bind_section_xform<Scalar, N>{scheme_});
      });
}

template <typename Scalar, size_t N, typename PrepareSections,
          typename ExecuteSections>
class openmp_scheme {
  using scheme_type = ::prtcl::data::scheme<Scalar, N>;
  using omp_scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

  using prepared_prepare_sections = decltype(openmp_prepare(
      std::declval<PrepareSections>(), std::declval<scheme_type &>()));
  using prepared_execute_sections = decltype(openmp_prepare(
      std::declval<ExecuteSections>(), std::declval<scheme_type &>()));

public:
  void prepare() {
    prtcl::cubic_spline_kernel<prtcl::host_math_traits<Scalar, N>> kernel;
    auto support_radius = kernel.get_support_radius(
        _scheme.get(tag::type::scalar{})["smoothing_scale"]);
    _grid.set_radius(support_radius);

    omp_scheme_type omp{_scheme};
    _grid.update(omp);
    boost::hana::for_each(
        openmp_bind(*_prepared_prepare_sections, omp),
        [&grid = _grid](auto section_) { section_.evaluate(grid); });
  }

  void execute() {
    omp_scheme_type omp{_scheme};
    _grid.update(omp);
    boost::hana::for_each(
        openmp_bind(*_prepared_execute_sections, omp),
        [&grid = _grid](auto section_) { section_.evaluate(grid); });
  }

public:
  template <typename PrepareSections_, typename ExecuteSections_>
  openmp_scheme(scheme_type &scheme_, PrepareSections_ &&prepare_sections_,
                ExecuteSections_ &&execute_sections_)
      : _scheme{scheme_}, _prepare_sections{std::forward<PrepareSections_>(
                              prepare_sections_)},
        _execute_sections{std::forward<ExecuteSections_>(execute_sections_)} {
    // fullfill the requirements of prepare and execute sections
    boost::hana::for_each(
        _prepare_sections, [&_scheme = _scheme](auto &&section_) {
          _scheme.fullfill_requirements(
              prtcl::expr::collect_scheme_requirements(section_));
        });
    boost::hana::for_each(
        _execute_sections, [&_scheme = _scheme](auto &&section_) {
          _scheme.fullfill_requirements(
              prtcl::expr::collect_scheme_requirements(section_));
        });
    // prepare the two pieces
    _prepared_prepare_sections = std::make_unique<prepared_prepare_sections>(
        openmp_prepare(_prepare_sections, _scheme));
    _prepared_execute_sections = std::make_unique<prepared_execute_sections>(
        openmp_prepare(_execute_sections, _scheme));
  }

private:
  scheme_type &_scheme;
  PrepareSections _prepare_sections;
  ExecuteSections _execute_sections;
  std::unique_ptr<prepared_prepare_sections> _prepared_prepare_sections;
  std::unique_ptr<prepared_execute_sections> _prepared_execute_sections;
  prtcl::grouped_uniform_grid<Scalar, N> _grid;
};

template <typename Scalar, size_t N, typename... Args>
auto make_openmp_scheme(::prtcl::data::scheme<Scalar, N> &scheme_,
                        Args &&... args_) {
  return openmp_scheme<Scalar, N, meta::remove_cvref_t<Args>...>{
      scheme_, std::forward<Args>(args_)...};
}

} // namespace prtcl::scheme
