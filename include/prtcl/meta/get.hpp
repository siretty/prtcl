#pragma once

#include <array>
#include <utility>

#include <cstddef>

namespace prtcl::meta {

template <size_t Index, typename T, size_t N>
constexpr auto get(std::array<T, N> const &a) {
  return a[Index];
}

template <size_t Index, typename I, I... Is>
constexpr auto get(std::integer_sequence<I, Is...>) {
  return std::array<I, sizeof...(Is)>{Is...}[Index];
}

} // namespace prtcl::meta
