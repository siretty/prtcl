#include <catch.hpp>

#include <algorithm>
#include <type_traits>

#include <prtcl/data/host/host_linear_data.hpp>
#include <prtcl/data/sycl/sycl_linear_access.hpp>

template <typename T, typename Data> void test_host_access(Data &data) {
  using namespace prtcl;

  auto buf = get_buffer(data, tag::sycl{});
  REQUIRE(10 == buf.size());

  auto rw = get_rw_access(buf);
  REQUIRE(std::is_same<decltype(rw[0]), T &>::value);
  REQUIRE(10 == rw.size());
  std::fill(rw.begin(), rw.end(), 1234);
  REQUIRE(std::all_of(rw.begin(), rw.end(), [](auto v) { return v == 1234; }));

  auto ro = get_ro_access(buf);
  REQUIRE(std::is_same<decltype(ro[0]), T>::value);
  REQUIRE(10 == ro.size());
  REQUIRE(std::all_of(ro.begin(), ro.end(), [](auto v) { return v == 1234; }));
}

template <typename T, typename Data> void test_device_access(Data &data) {
  using namespace prtcl;

  {
    sycl::queue queue;

    auto buf = get_buffer(data, tag::sycl{});

    queue.submit([&](auto &cgh) {
      auto rw = get_rw_access(buf, cgh);

      cgh.template parallel_for<class testing>(
          sycl::range<1>{buf.size()},
          [=](sycl::item<1> id) { rw[id[0]] = static_cast<T>(id[0]); });
    });
  }

  for (size_t pos = 0; pos < data.size(); ++pos)
    REQUIRE(static_cast<T>(pos) == data[pos]);
}

TEST_CASE("prtcl::sycl_linear_access", "[prtcl]") {
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

  test_host_access<T>(data);
  test_device_access<T>(data);
}
