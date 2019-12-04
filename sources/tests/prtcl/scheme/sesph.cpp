#include <catch.hpp>

#include <prtcl/expr/print.hpp>
#include <prtcl/scheme/sesph.hpp>

TEST_CASE("prtcl/scheme/sesph", "[prtcl][scheme][sesph]") {
  std::cerr << "<prtcl>" << std::endl;
  prtcl::expr::print(std::cerr, prtcl::scheme::sesph_density_pressure());
  prtcl::expr::print(std::cerr, prtcl::scheme::sesph_acceleration());
  prtcl::expr::print(std::cerr, prtcl::scheme::sesph_symplectic_euler());
  std::cerr << "</prtcl>" << std::endl;
}
