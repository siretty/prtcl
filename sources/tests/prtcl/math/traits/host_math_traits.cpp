#include <catch.hpp>

#include "common_math_traits.hpp"

#include <prtcl/math/traits/host_math_traits.hpp>

TEST_CASE("prtcl/math/traits/host_math_traits", "[prtcl][math][traits][host]") {
  common_math_traits_tests<prtcl::host_math_traits<float, 3>>();
}
