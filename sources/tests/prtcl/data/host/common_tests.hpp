#pragma once

#include <catch.hpp>

#include <prtcl/tags.hpp>

namespace prtcl_tests {

template <typename T, size_t N, template <typename> typename LinearDataT,
          template <typename, size_t, typename> typename ArrayDataT>
void common_host_array_data() {
  ArrayDataT<T, N, LinearDataT<T>> data;

  // initial size and empty buffer
  REQUIRE(0 == data.size());

  // resize the data
  data.resize(10);
  REQUIRE(10 == data.size());

  SECTION("buffer") {
    auto buf = get_buffer(data, prtcl::tag::host{});
    REQUIRE(10 == buf.size());

    std::array<T, N> value;
    value.fill(1234);

    SECTION("rw access") {
      { // fill the buffer with some data
        auto rw = get_rw_access(buf);
        REQUIRE(10 == rw.size());

        for (size_t pos = 0; pos < rw.size(); ++pos)
          rw.set(pos, value);
        for (size_t pos = 0; pos < rw.size(); ++pos)
          REQUIRE(value == rw.get(pos));
      }

      SECTION("ro access") {
        auto ro = get_ro_access(buf);
        REQUIRE(10 == ro.size());

        // must not compile
        // for (size_t pos = 0; pos < ro.size(); ++pos)
        //  ro.set(pos, value);
        for (size_t pos = 0; pos < ro.size(); ++pos)
          REQUIRE(value == ro.get(pos));
      }
    }
  }
}

template <typename T, template <typename> typename LinearDataT,
          template <typename, typename> typename ScalarDataT>
void common_host_scalar_data() {
  ScalarDataT<T, LinearDataT<T>> data;

  // initial size and empty buffer
  REQUIRE(0 == data.size());

  // resize the data
  data.resize(10);
  REQUIRE(10 == data.size());

  SECTION("buffer") {
    auto buf = get_buffer(data, prtcl::tag::host{});
    REQUIRE(10 == buf.size());

    T value = 1234;

    SECTION("rw access") {
      { // fill the buffer with some data
        auto rw = get_rw_access(buf);
        REQUIRE(10 == rw.size());

        for (size_t pos = 0; pos < rw.size(); ++pos)
          rw.set(pos, value);
        for (size_t pos = 0; pos < rw.size(); ++pos)
          REQUIRE(value == rw.get(pos));
      }

      SECTION("ro access") {
        auto ro = get_ro_access(buf);
        REQUIRE(10 == ro.size());

        // must not compile
        // for (size_t pos = 0; pos < ro.size(); ++pos)
        //  ro.set(pos, value);
        for (size_t pos = 0; pos < ro.size(); ++pos)
          REQUIRE(value == ro.get(pos));
      }
    }
  }
}

} // namespace prtcl_tests
