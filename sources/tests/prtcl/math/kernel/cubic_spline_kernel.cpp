#include <catch.hpp>

#include "common_kernel.hpp"

#include <prtcl/math/kernel/cubic_spline_kernel.hpp>
#include <prtcl/math/traits/host_math_traits.hpp>

#include <boost/type_index.hpp>

namespace {

template <typename T, size_t N, typename F> void run_tests(F &&f) {
  SECTION(boost::typeindex::type_id<T>().pretty_name()) {
    SECTION(std::to_string(N)) {
      using math_traits = prtcl::host_math_traits<T, N>;
      std::invoke(f, math_traits{});
    }
  }
}

} // namespace

TEST_CASE("prtcl/math/kernel/cubic_spline_kernel",
          "[prtcl][math][kernel][cubic_spline_kernel]") {
  auto _tests = [](auto _traits) {
    using math_traits = decltype(_traits);
    common_math_kernel_tests<prtcl::cubic_spline_kernel<math_traits>>();
  };

  run_tests<float, 1>(_tests);
  run_tests<float, 2>(_tests);
  run_tests<float, 3>(_tests);

  run_tests<double, 1>(_tests);
  run_tests<double, 2>(_tests);
  run_tests<double, 3>(_tests);
}
