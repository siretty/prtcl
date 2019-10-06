#include <catch.hpp>

#include <algorithm>

#include <prtcl/data/array_of_vectors.hpp>
#include <prtcl/data/host/host_linear_data.hpp>
#include <prtcl/data/vector_of_arrays.hpp>

#include "../host_data.hpp"

TEST_CASE("prtcl::host_linear_buffer", "[prtcl]") {
  using namespace prtcl;

  using T = double;

  host_linear_data<T> buf;

  // initial size and empty buffer
  REQUIRE(0 == buf.size());
  REQUIRE(nullptr == buf.data());

  SECTION("buf.resize(...)") {
    buf.resize(10);
    REQUIRE(10 == buf.size());

    // some test data
    std::vector<T> data(buf.size());
    std::iota(data.begin(), data.end(), 0);

    SECTION("buf[...]") {
      for (size_t pos = 0; pos < buf.size(); ++pos)
        buf[pos] = data[pos];

      for (size_t pos = 0; pos < buf.size(); ++pos)
        REQUIRE(data[pos] == buf[pos]);
    }

    SECTION("buf.begin(), buf.end()") {
      std::copy(data.begin(), data.end(), buf.begin());
      REQUIRE(std::equal(data.begin(), data.end(), buf.begin(), buf.end()));
    }

    SECTION("buf.resize(...), larger") {
      buf.resize(20);
      REQUIRE(20 == buf.size());
    }

    SECTION("buf.resize(...), smaller") {
      buf.resize(5);
      REQUIRE(5 == buf.size());
    }
  }
}
