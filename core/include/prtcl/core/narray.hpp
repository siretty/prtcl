#pragma once

#include <array>
#include <type_traits>

#include <cstddef>

namespace prtcl::core {

// {{{ implementation detail
namespace details {

// ------------------------------------------------------------
// Nested Array Type-Generator
// ------------------------------------------------------------

template <typename, size_t...> struct narray;

template <typename T_, size_t N0_, size_t... Ns_>
struct narray<T_, N0_, Ns_...> {
  using type = std::array<typename narray<T_, Ns_...>::type, N0_>;
};

template <typename T_> struct narray<T_> { using type = T_; };

// ------------------------------------------------------------
// Nested Array Size-Trait
// ------------------------------------------------------------

template <typename T_>
struct narray_size : std::integral_constant<size_t, 1> {};

template <typename T_, size_t N_>
struct narray_size<std::array<T_, N_>>
    : std::integral_constant<size_t, N_ * narray_size<T_>::value> {};

// ------------------------------------------------------------
// Nested Array Element Type-Trait
// ------------------------------------------------------------

template <typename T_> struct narray_element_type { using type = T_; };

template <typename T_, size_t N_>
struct narray_element_type<std::array<T_, N_>> {
  using type = typename narray_element_type<T_>::type;
};

} // namespace details
// }}}

// ============================================================
// Type Alias
// ============================================================

/// Type alias for nested std::array's.
/// The memory layout is compatible with multi-dimensional C-arrays.
template <typename T_, size_t... Ns_>
using narray_t = typename details::narray<T_, Ns_...>::type;

template <typename T_>
using narray_element_t = typename details::narray_element_type<T_>::type;

// ============================================================
// Generic Accessor Functions
// ============================================================

/// Returns a pointer to the first element of an ndarray.
/// This overload handles non-array types (ie. ndarray elements).
template <typename T_> auto *narray_data(T_ &ref_) { return &ref_; }

/// Returns a pointer to the first element of an ndarray.
/// This overload handles array types by unwrapping the array.
template <typename T_, size_t N_>
auto *narray_data(std::array<T_, N_> &array_) {
  return narray_data(array_[0]);
}

template <typename T_> constexpr size_t narray_size() {
  return details::narray_size<T_>::value;
}

template <typename T_> constexpr size_t narray_size(T_ &) {
  return details::narray_size<T_>::value;
}

// TODO: constexpr narray_rank, constexpr narray_extent

} // namespace prtcl::core
