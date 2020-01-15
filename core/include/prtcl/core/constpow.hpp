#pragma once

#include <cstddef>

#include <type_traits>

namespace prtcl::core {

template <typename T, typename U> constexpr T constpow(T base, U exponent) {
  static_assert(std::is_arithmetic<T>::value);
  static_assert(std::is_integral<U>::value);

  T result = static_cast<T>(1);
  for (; exponent > 0; --exponent)
    result *= base;
  return result;
}

} // namespace prtcl::core
