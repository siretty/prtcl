#pragma once

#include <catch.hpp>

#include <prtcl/tags/sycl.hpp>

namespace tests::prtcl {

template <typename Data> inline void common_host_data_tests(Data &data) {
  REQUIRE(0 == data.size());

  data.resize(10);
  REQUIRE(10 == data.size());
  // REQUIRE(10 <= data.capacity());

  // SECTION("reserve increases the capacity") {
  //  auto old_capacity = data.capacity();
  //  data.reserve(old_capacity * 2);
  //  REQUIRE(old_capacity * 2 <= data.capacity());
  //}

  SECTION("testing get and set") {
    data.resize(10);
    REQUIRE(0 < data.size());

    for (size_t n = 0; n < data.size(); ++n) {
      data.set(n, {n + 1., n + 2., n + 3.});
      auto value = data.get(n);
      REQUIRE(value[0] == n + 1.);
      REQUIRE(value[1] == n + 2.);
      REQUIRE(value[2] == n + 3.);
    }
  }
}

template <typename Data> inline void common_host_data_tests() {
  Data data;
  common_host_data_tests<Data>(data);
}

template <typename T, size_t N, template <typename> typename LinearDataT,
          template <typename, size_t, typename> typename ArrayDataT>
void test_common_host_array_data() {
  ArrayDataT<T, N, LinearDataT<T>> data;

  // initial size and empty buffer
  REQUIRE(0 == data.size());

  // resize the data
  data.resize(10);
  REQUIRE(10 == data.size());

  SECTION("buffer") {
    auto buf = get_buffer(data);
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
void test_common_host_scalar_data() {
  ScalarDataT<T, LinearDataT<T>> data;

  // initial size and empty buffer
  REQUIRE(0 == data.size());

  // resize the data
  data.resize(10);
  REQUIRE(10 == data.size());

  SECTION("buffer") {
    auto buf = get_buffer(data);
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

} // namespace tests::prtcl
