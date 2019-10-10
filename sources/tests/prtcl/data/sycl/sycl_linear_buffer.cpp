#include <catch.hpp>

#include <algorithm>
#include <type_traits>

#include <prtcl/data/host/host_linear_data.hpp>
#include <prtcl/data/sycl/sycl_linear_buffer.hpp>

TEST_CASE("prtcl::sycl_linear_buffer", "[prtcl]") {
  using namespace prtcl;

  using T = double;

  host_linear_data<T> data;

  // initial size and empty buffer
  REQUIRE(0 == data.size());
  REQUIRE(nullptr == data.data());

  // resize the data
  data.resize(10);
  REQUIRE(10 == data.size());
  REQUIRE(nullptr != data.data());

  auto buf = get_buffer(data, tag::sycl{});
  REQUIRE(10 == buf.size());
}
