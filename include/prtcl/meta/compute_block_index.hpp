#pragma once

#include <prtcl/meta/get.hpp>
#include <prtcl/meta/size.hpp>
#include <prtcl/meta/unpack_integer_sequence.hpp>

#include <array>
#include <utility>

#include <cstddef>

namespace prtcl::detail {

template <size_t Index, typename Shape>
constexpr size_t compute_c_stride(Shape const &shape_) {
  static_assert(0 <= Index and Index < meta::size_v<Shape>);
  if constexpr (meta::size_v<Shape> - 1 == Index)
    return 1;
  else
    return meta::get<Index + 1>(shape_) * compute_c_stride<Index + 1>(shape_);
}

template <typename Shape> struct c_strides;

template <typename I, I... Is>
struct c_strides<std::integer_sequence<I, Is...>> {
private:
  static constexpr std::array<size_t, sizeof...(Is)> _shape{Is...};

  template <typename> struct impl;

  template <size_t... Ns> struct impl<std::index_sequence<Ns...>> {
    using type = std::integer_sequence<I, compute_c_stride<Ns>(_shape)...>;
  };

public:
  using type = typename impl<std::make_index_sequence<sizeof...(Is)>>::type;
};

} // namespace prtcl::detail

namespace prtcl::meta {

template <typename Shape, typename MultiIndex>
size_t linear_block_index(Shape, MultiIndex) {
  using strides_type = typename detail::c_strides<Shape>::type;
  return meta::unpack_integer_sequences(
      [](auto... args) { return ((args.first * args.second) + ... + 0); },
      strides_type{}, MultiIndex{});
}

} // namespace prtcl::meta
