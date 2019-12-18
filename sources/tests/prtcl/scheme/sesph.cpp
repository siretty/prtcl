#include <catch.hpp>

#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/print.hpp>
#include <prtcl/expr/scheme_requirements.hpp>
#include <prtcl/scheme/sesph.hpp>

TEST_CASE("prtcl/scheme/sesph", "[prtcl][scheme][sesph]") {
  auto const sesph_density_pressure = prtcl::scheme::sesph_density_pressure();
  auto const sesph_acceleration = prtcl::scheme::sesph_acceleration();
  auto const sesph_symplectic_euler = prtcl::scheme::sesph_symplectic_euler();

  std::cerr << "<prtcl>" << std::endl;
  prtcl::expr::print(std::cerr, sesph_density_pressure);
  prtcl::expr::print(std::cerr, sesph_acceleration);
  prtcl::expr::print(std::cerr, sesph_symplectic_euler);
  std::cerr << "</prtcl>" << std::endl;

  auto print_required_fields = [](auto &stream_, auto &&s) {
    auto print_field = [&stream_](auto const &f) {
      stream_ << "    " << f.kind_tag << " " << f.type_tag << " " << f.value
              << std::endl;
    };
    stream_ << "required fields:" << std::endl;
    auto const reqs =
        prtcl::expr::collect_scheme_requirements(std::forward<decltype(s)>(s));
    stream_ << "  globals:" << std::endl;
    for (auto const &fvar : reqs.globals)
      std::visit(print_field, fvar);
    stream_ << "  uniforms:" << std::endl;
    for (auto const &[_, fvar] : reqs.uniforms)
      std::visit(print_field, fvar);
    stream_ << "  varyings:" << std::endl;
    for (auto const &[_, fvar] : reqs.varyings)
      std::visit(print_field, fvar);
  };

  std::cerr << "<!--" << std::endl;
  print_required_fields(std::cerr, sesph_density_pressure);
  print_required_fields(std::cerr, sesph_acceleration);
  print_required_fields(std::cerr, sesph_symplectic_euler);
  std::cerr << "-->" << std::endl;

  prtcl::data::scheme<float, 3> scheme;
  scheme.add_group("other").add_flag("other");
  scheme.add_group("fluid-a").add_flag("fluid");
  scheme.add_group("fluid-b").add_flag("fluid");
  scheme.add_group("boundary-a").add_flag("boundary");
  scheme.add_group("boundary-b").add_flag("boundary");

  scheme.fullfill_requirements(
      prtcl::expr::collect_scheme_requirements(sesph_density_pressure));
  scheme.fullfill_requirements(
      prtcl::expr::collect_scheme_requirements(sesph_acceleration));
  scheme.fullfill_requirements(
      prtcl::expr::collect_scheme_requirements(sesph_symplectic_euler));

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
}