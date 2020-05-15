#pragma once

#include <type_traits>

namespace prtcl::rt {

template <typename> struct is_model_policy : std::false_type {};

template <typename Type_>
constexpr bool is_model_policy_v = is_model_policy<Type_>::value;

} // namespace prtcl::rt
