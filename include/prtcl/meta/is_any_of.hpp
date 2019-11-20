#pragma once

#include <type_traits>

namespace prtcl {

template <typename T, typename... Us>
using is_any_of = std::disjunction<std::is_same<T, Us>...>;

template <typename T, typename... Us>
constexpr bool is_any_of_v = is_any_of<T, Us...>::value;

} // namespace prtcl

namespace prtcl::meta {

using ::prtcl::is_any_of;
using ::prtcl::is_any_of_v;

// is_any_type_of

template <typename T, typename... Us>
struct is_any_type_of : std::disjunction<std::is_same<T, Us>...> {};

template <typename T, typename... Us>
constexpr bool is_any_type_of_v = is_any_type_of_v<T, Us...>;

// is_any_value_of

template <typename T, T Value, T... Cases>
struct is_any_value_of
    : std::disjunction<std::integral_constant<bool, Value == Cases>...> {};

template <typename T, T Value, T... Cases>
constexpr bool is_any_value_of_v = is_any_value_of<T, Value, Cases...>::value;

} // namespace prtcl::meta
