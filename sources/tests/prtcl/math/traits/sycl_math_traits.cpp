#include <catch.hpp>

#include "common_math_traits.hpp"

#include <prtcl/math/traits/sycl_math_traits.hpp>

TEST_CASE("prtcl/math/traits/sycl_math_traits", "[prtcl][math][traits][sycl]") {
  common_math_traits_tests<prtcl::sycl_math_traits<float, 3>>();
}
