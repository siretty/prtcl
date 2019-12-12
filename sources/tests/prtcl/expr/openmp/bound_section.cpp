#include <catch.hpp>

#include <prtcl/data/openmp/scheme.hpp>
#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/openmp/bind_field.hpp>
#include <prtcl/expr/openmp/bound_section.hpp>
#include <prtcl/expr/openmp/prepared_loop.hpp>
#include <prtcl/expr/scheme_requirements.hpp>
#include <prtcl/meta/format_cxx_type.hpp>
#include <prtcl/scheme/sesph.hpp>

TEST_CASE("prtcl/expr/openmp/bound_section",
          "[prtcl][expr][openmp][bound_section]") {
  auto const sesph_density_pressure = prtcl::scheme::sesph_density_pressure();

  prtcl::data::scheme<float, 3> scheme;
  scheme.add_group("other").add_flag("other").resize(1);
  scheme.add_group("fluid-a").add_flag("fluid").resize(2);
  scheme.add_group("fluid-b").add_flag("fluid").resize(3);
  scheme.add_group("boundary-a").add_flag("boundary").resize(4);
  scheme.add_group("boundary-b").add_flag("boundary").resize(5);

  scheme.fullfill_requirements(
      prtcl::expr::collect_scheme_requirements(sesph_density_pressure));

  prtcl::data::openmp::scheme<float, 3> omp_scheme{scheme};

  auto e0 = boost::yap::transform(
      sesph_density_pressure,
      prtcl::expr::openmp::prepared_particle_loop_xform<float, 3>{scheme});
  auto e1 = boost::yap::transform(
      e0, prtcl::expr::openmp::bind_section_xform<float, 3>{omp_scheme});

  std::cerr << "<!--" << std::endl;
  display_cxx_type(e1, std::cerr);
  std::cerr << "-->" << std::endl;
}
