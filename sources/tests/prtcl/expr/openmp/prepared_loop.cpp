#include <catch.hpp>

#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/openmp/prepared_loop.hpp>
#include <prtcl/expr/scheme_requirements.hpp>
#include <prtcl/meta/format_cxx_type.hpp>
#include <prtcl/scheme/sesph.hpp>

TEST_CASE("prtcl/expr/openmp/prepared_loop", "[prtcl][expr][openmp][prepared_loop]") {
  auto const sesph_density_pressure = prtcl::scheme::sesph_density_pressure();

  prtcl::data::scheme<float, 3> scheme;
  scheme.add_group("other").add_flag("other");
  scheme.add_group("fluid-a").add_flag("fluid");
  scheme.add_group("fluid-b").add_flag("fluid");
  scheme.add_group("boundary-a").add_flag("boundary");
  scheme.add_group("boundary-b").add_flag("boundary");

  scheme.fullfill_requirements(
      prtcl::expr::collect_scheme_requirements(sesph_density_pressure));

  std::cerr << "<!--" << std::endl;
  {
    namespace ktag = ::prtcl::tag::kind;
    namespace ttag = ::prtcl::tag::type;
    for (auto name : scheme.get(ttag::scalar{}).names())
      std::cerr << "global scalar: " << name << std::endl;
    for (auto name : scheme.get(ttag::vector{}).names())
      std::cerr << "global vector: " << name << std::endl;
    for (auto name : scheme.get(ttag::matrix{}).names())
      std::cerr << "global matrix: " << name << std::endl;
    for (size_t gi = 0; gi < scheme.get_group_count(); ++gi) {
      std::cerr << "group #" << gi << " \"" << scheme.get_group_name(gi).value()
                << "\":" << std::endl;
      auto &g = scheme.get_group(gi);
      for (auto name : g.get(ktag::uniform{}, ttag::scalar{}).names())
        std::cerr << "  uniform scalar: " << name << std::endl;
      for (auto name : g.get(ktag::uniform{}, ttag::vector{}).names())
        std::cerr << "  uniform vector: " << name << std::endl;
      for (auto name : g.get(ktag::uniform{}, ttag::matrix{}).names())
        std::cerr << "  uniform matrix: " << name << std::endl;
      for (auto name : g.get(ktag::varying{}, ttag::scalar{}).names())
        std::cerr << "  varying scalar: " << name << std::endl;
      for (auto name : g.get(ktag::varying{}, ttag::vector{}).names())
        std::cerr << "  varying vector: " << name << std::endl;
      for (auto name : g.get(ktag::varying{}, ttag::matrix{}).names())
        std::cerr << "  varying matrix: " << name << std::endl;
    }
  }
  std::cerr << "-->" << std::endl;

  auto e = boost::yap::transform(
      sesph_density_pressure,
      prtcl::expr::openmp::prepared_particle_loop_xform<float, 3>{scheme});

  std::cerr << "<!--" << std::endl;
  display_cxx_type(e, std::cerr);
  std::cerr << "-->" << std::endl;
}
