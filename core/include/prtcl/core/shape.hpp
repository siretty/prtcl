#pragma once

#include <array>
#include <tuple>
#include <type_traits>

#include <cstddef>

namespace prtcl {

template <size_t... Extents_> class shape {
public:
  static constexpr size_t rank() noexcept { return sizeof...(Extents_); }

  template <size_t Index_> static constexpr size_t get() noexcept {
    return std::array<size_t, rank()>{Extents_...}[Index_];
  }

  template <size_t ModelExtent_>
  static constexpr auto replace_zero_extents() noexcept {
    return shape<((0 == Extents_) ? ModelExtent_ : Extents_)...>{};
  }

  template <size_t ModelExtent_>
  using replace_zero_extents_t = decltype(replace_zero_extents<ModelExtent_>());

  static constexpr bool has_zero_extents() noexcept {
    return ((0 == Extents_) || ...);
  }

public:
  template <size_t... OtherExtents_>
  constexpr bool operator==(shape<OtherExtents_...> rhs_) const {
    if constexpr (rank() == rhs_.rank())
      return ((Extents_ == OtherExtents_) && ...);
    else
      return false;
  }

  template <size_t... OtherExtents_>
  constexpr bool operator!=(shape<OtherExtents_...> rhs_) const {
    return not(*this == rhs_);
  }
};

template <typename> struct is_shape : std::false_type {};

template <size_t... Extents_>
struct is_shape<shape<Extents_...>> : std::true_type {};

template <typename T> constexpr bool is_shape_v = is_shape<T>::value;

} // namespace prtcl

namespace std {

template <size_t... Extents_>
struct tuple_size<::prtcl::shape<Extents_...>>
    : std::integral_constant<size_t, sizeof...(Extents_)> {};

template <std::size_t Index_, size_t... Extents_>
struct tuple_element<Index_, ::prtcl::shape<Extents_...>> {
  using type = size_t;
};

} // namespace std
