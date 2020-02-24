#include <catch2/catch.hpp>

#include <prtcl/core/narray.hpp>

template <typename T_, size_t... Ns_>
void yoink(prtcl::core::narray_t<T_, Ns_...> &&t_) {
  (void)(t_);
}

TEST_CASE("prtcl/core/ndarray", "[prtcl][core]") {
  using prtcl::core::narray_t, prtcl::core::narray_size;

  auto extent_of = [](auto &&arg) {
    using type = std::remove_reference_t<std::remove_cv_t<decltype(arg)>>;
    return std::extent<type, 0>::value;
  };

  constexpr int m = 2, n = 3;

  int a[m][n] = {
      {0, 1, 2},
      {3, 4, 5},
  };
  CHECK(m == extent_of(a));
  CHECK(n == extent_of(a[0]));
  CHECK(n == (&a[1][0] - &a[0][0]));

  for (int i = 0; i < m * n; ++i) {
    CHECK(i == (&a[0][0])[i]);
  }

  auto b = narray_t<int, m, n>{{
      {0, 1, 2},
      {3, 4, 5},
  }};
  CHECK(m == b.size());
  CHECK(n == b[0].size());
  CHECK(n == (&b[1][0] - &b[0][0]));
  CHECK(m * n == narray_size(b));

  for (int i = 0; i < m * n; ++i) {
    CHECK(i == (&b[0][0])[i]);
  }

  yoink<int>({0});
  yoink<int, m, n>({{{0, 1, 2}, {3, 4, 5}}});
}
