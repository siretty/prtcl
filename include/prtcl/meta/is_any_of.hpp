#pragma once

#include <type_traits>

namespace prtcl {

template <typename T, typename... Us>
using is_any_of = std::disjunction<std::is_same<T, Us>...>;

template <typename T, typename... Us>
constexpr bool is_any_of_v = is_any_of<T, Us...>::value;

} // namespace prtcl
