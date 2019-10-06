#include <catch.hpp>

#include <prtcl/data/host/host_vector_of_arrays.hpp>

#include <tests/prtcl/data/host_data.hpp>

TEST_CASE("prtcl::host_vector_of_arrays", "[prtcl]") {
  tests::prtcl::common_host_data_tests<
      prtcl::host_vector_of_arrays<double, 3>>();
}
