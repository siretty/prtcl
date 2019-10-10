#include <catch.hpp>

#include <algorithm>
#include <type_traits>

#include <prtcl/data/host/host_linear_access.hpp>
#include <prtcl/data/host/host_linear_data.hpp>

TEST_CASE("prtcl::host_linear_access", "[prtcl]") {
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

  auto buf = get_buffer(data, tag::host{});
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
