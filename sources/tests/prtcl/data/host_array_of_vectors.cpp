#include <catch.hpp>

#include <prtcl/data/host/host_array_of_vectors.hpp>

#include <tests/prtcl/data/host_data.hpp>

TEST_CASE("prtcl::host_array_of_vectors", "[prtcl]") {
  tests::prtcl::common_host_data_tests<
      prtcl::host_array_of_vectors<double, 3>>();
}
