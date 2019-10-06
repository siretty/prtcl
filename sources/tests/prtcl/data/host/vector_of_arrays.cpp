#include <catch.hpp>

#include <algorithm>
#include <type_traits>

#include <prtcl/data/host/host_linear_access.hpp>
#include <prtcl/data/host/host_linear_data.hpp>
#include <prtcl/data/vector_of_arrays.hpp>

#include "common_tests.hpp"

TEST_CASE("prtcl::vector_of_arrays_... host", "[prtcl][host]") {
  using namespace prtcl;

  SECTION("float, 1") {
    prtcl_tests::common_host_array_data<float, 1, host_linear_data,
                                        vector_of_arrays_data>();
  }

  SECTION("float, 2") {
    prtcl_tests::common_host_array_data<float, 2, host_linear_data,
                                        vector_of_arrays_data>();
  }

  SECTION("float, 3") {
    prtcl_tests::common_host_array_data<float, 3, host_linear_data,
                                        vector_of_arrays_data>();
  }

  SECTION("double, 1") {
    prtcl_tests::common_host_array_data<double, 1, host_linear_data,
                                        vector_of_arrays_data>();
  }

  SECTION("double, 2") {
    prtcl_tests::common_host_array_data<double, 2, host_linear_data,
                                        vector_of_arrays_data>();
  }

  SECTION("double, 3") {
    prtcl_tests::common_host_array_data<double, 3, host_linear_data,
                                        vector_of_arrays_data>();
  }
}
