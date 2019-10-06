#include <catch.hpp>

#include <algorithm>
#include <type_traits>

#include <prtcl/data/host/host_linear_data.hpp>
#include <prtcl/data/host/host_linear_access.hpp>
#include <prtcl/data/sycl/sycl_linear_access.hpp>
#include <prtcl/data/sycl/sycl_linear_buffer.hpp>
#include <prtcl/data/vector_of_scalars.hpp>

#include "common_tests.hpp"

TEST_CASE("prtcl::vector_of_scalars_... sycl", "[prtcl][sycl]") {
  using namespace prtcl;

  SECTION("signed int") {
    prtcl_tests::common_sycl_scalar_data<signed int, host_linear_data,
                                         vector_of_scalars_data>();
  }

  SECTION("unsigned int") {
    prtcl_tests::common_sycl_scalar_data<unsigned int, host_linear_data,
                                         vector_of_scalars_data>();
  }

  SECTION("signed long long") {
    prtcl_tests::common_sycl_scalar_data<signed long long, host_linear_data,
                                         vector_of_scalars_data>();
  }

  SECTION("unsigned long long") {
    prtcl_tests::common_sycl_scalar_data<unsigned long long, host_linear_data,
                                         vector_of_scalars_data>();
  }

  SECTION("float") {
    prtcl_tests::common_sycl_scalar_data<float, host_linear_data,
                                         vector_of_scalars_data>();
  }

  SECTION("double") {
    prtcl_tests::common_sycl_scalar_data<double, host_linear_data,
                                         vector_of_scalars_data>();
  }
}

// vim: set foldmethod=marker:
