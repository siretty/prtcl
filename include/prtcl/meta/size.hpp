#pragma once

#include <prtcl/meta/remove_cvref.hpp>

#include <array>
#include <type_traits>
#include <utility>

#include <cstddef>

namespace prtcl::meta {

template <typename> struct size;

template <typename T, size_t N>
struct size<std::array<T, N>> : std::integral_constant<size_t, N> {};

template <typename I, I... Is>
struct size<std::integer_sequence<I, Is...>>
    : std::integral_constant<size_t, sizeof...(Is)> {};

template <typename T> constexpr size_t size_v = size<remove_cvref_t<T>>::value;

} // namespace prtcl::meta
