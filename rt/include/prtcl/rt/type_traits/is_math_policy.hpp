#pragma once

#include <type_traits>

namespace prtcl::rt {

template <typename> struct is_math_policy : std::false_type {};

template <typename Type_>
constexpr bool is_math_policy_v = is_math_policy<Type_>::value;

} // namespace prtcl::rt
