#pragma once

#include <catch.hpp>

#include <prtcl/libs/sycl.hpp>
#include <prtcl/tags/sycl.hpp>

#include <sstream>
#include <string>

namespace prtcl_tests {

template <typename T, size_t N, template <typename> typename LinearDataT,
          template <typename, size_t, typename> typename ArrayDataT>
void common_sycl_array_data() {
  ArrayDataT<T, N, LinearDataT<T>> data;

  // initial size and empty buffer
  REQUIRE(0 == data.size());

  // resize the data
  data.resize(10);
  REQUIRE(10 == data.size());

  SECTION("buffer") {
    auto buf = get_buffer(data, ::prtcl::tags::sycl{});
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

  /*
  SECTION("sycl") {
    std::array<T, N> value;
    value.fill(1234);

    SECTION("rw access") {
      { // fill the buffer with some data
        sycl::queue queue;

        auto buf = get_buffer(data, ::prtcl::tags::sycl{});
        REQUIRE(10 == buf.size());

        queue.submit([&](auto &cgh) {
          auto rw = get_rw_access(buf);
          REQUIRE(10 == rw.size());

          cgh.template parallel_for<
              class prtcl_tests_data_sycl_common_scalar_get_rw_access>(
              rw.size(), [=](sycl::item<1> id) { rw.set(id[0], value); });
        });
      }

      // check using a host accessor
      auto buf = get_buffer(data, ::prtcl::tags::sycl{});
      auto ro = get_ro_access(buf);

      for (size_t pos = 0; pos < data.size(); ++pos)
        REQUIRE(value == ro.get(pos));
    }

    SECTION("ro access") {
      { // fill the buffer with some data
        sycl::queue queue;

        auto buf = get_buffer(data, ::prtcl::tags::sycl{});
        REQUIRE(10 == buf.size());

        queue.submit([&](auto &cgh) {
          auto rw = get_rw_access(buf);
          REQUIRE(10 == rw.size());

          cgh.template parallel_for<
              class prtcl_tests_data_sycl_common_scalar_get_ro_access_prepare>(
              rw.size(), [=](sycl::item<1> id) { rw.set(id[0], value); });
        });

        queue.submit([&](auto &cgh) {
          auto ro = get_ro_access(buf);
          auto rw = get_rw_access(buf);
          REQUIRE(10 == ro.size());
          REQUIRE(10 == rw.size());

          cgh.template parallel_for<
              class prtcl_tests_data_sycl_common_scalar_get_ro_access>(
              ro.size(), [=](sycl::item<1> id) {
                auto v = ro.get(id[0]);
                for (size_t n = 0; n < N; ++n)
                  v[n] *= 2;
                rw.set(id[0], v);
              });
        });
      }

      auto buf = get_buffer(data, ::prtcl::tags::sycl{});
      REQUIRE(10 == buf.size());

      // check using a host accessor
      auto ro = get_ro_access(buf);
      REQUIRE(10 == ro.size());

      for (size_t pos = 0; pos < ro.size(); ++pos)
        for (size_t n = 0; n < N; ++n)
          REQUIRE(2 * value[n] == ro.get(pos)[n]);
    }
  }
  */
}

template <typename... Args> std::string cat(Args &&... args) {
  std::ostringstream s;
  (void)(s << ... << std::forward<Args>(args));
  return s.str();
}

template <typename T, template <typename> typename LinearDataT,
          template <typename, typename> typename ScalarDataT>
void common_sycl_scalar_data() {
  ScalarDataT<T, LinearDataT<T>> data;

  // initial size and empty buffer
  REQUIRE(0 == data.size());

  // resize the data
  data.resize(10);
  REQUIRE(10 == data.size());

  SECTION("host") {
    auto buf = get_buffer(data, ::prtcl::tags::sycl{});
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

  SECTION("sycl") {
    T value = 1234;

    SECTION("rw access") {
      { // fill the buffer with some data
        sycl::queue queue;

        auto buf = get_buffer(data, ::prtcl::tags::sycl{});
        REQUIRE(10 == buf.size());

        queue.submit([&](auto &cgh) {
          auto rw = get_rw_access(buf);
          REQUIRE(10 == rw.size());

          cgh.template parallel_for<
              class prtcl_tests_data_sycl_common_scalar_get_rw_access>(
              rw.size(), [=](sycl::item<1> id) { rw.set(id[0], value); });
        });
      }

      // check using a host accessor
      auto buf = get_buffer(data, ::prtcl::tags::sycl{});
      auto ro = get_ro_access(buf);

      for (size_t pos = 0; pos < data.size(); ++pos)
        REQUIRE(value == ro.get(pos));
    }

    SECTION("ro access") {
      auto buf = get_buffer(data, ::prtcl::tags::sycl{});
      REQUIRE(10 == buf.size());

      ScalarDataT<T, LinearDataT<T>> data2;
      data2.resize(data.size());
      REQUIRE(data.size() == data2.size());

      auto buf2 = get_buffer(data2, ::prtcl::tags::sycl{});
      REQUIRE(10 == buf2.size());

      { // fill the buffer with some data
        sycl::queue queue;

        queue.submit([&](auto &cgh) {
          auto rw = get_rw_access(buf, cgh);
          REQUIRE(10 == rw.size());

          cgh.template parallel_for<
              class prtcl_tests_data_sycl_common_scalar_get_ro_access_prepare>(
              rw.size(), [=](sycl::item<1> id) {
                rw.set(id[0], value);
              });
        });

        queue.submit([&](auto &cgh) {
          auto ro = get_ro_access(buf, cgh);
          auto rw = get_rw_access(buf2, cgh);
          REQUIRE(10 == ro.size());
          REQUIRE(10 == rw.size());

          cgh.template parallel_for<
              class prtcl_tests_data_sycl_common_scalar_get_ro_access>(
              ro.size(), [=](sycl::item<1> id) {
                rw.set(id[0], ro.get(id[0]) * 2);
              });
        });
      }

      // check using a host accessor
      auto ro = get_ro_access(buf2);
      REQUIRE(10 == ro.size());

      for (size_t pos = 0; pos < ro.size(); ++pos)
        REQUIRE(2 * value == ro.get(pos));
    }
  }
}

} // namespace prtcl_tests
