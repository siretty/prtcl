#pragma once

#include <prtcl/rt/type_traits/is_model_policy.hpp>

#include <type_traits>

namespace prtcl::rt {

template <typename ModelPolicy_>
using type_policy_t = typename ModelPolicy_::type_policy;

template <typename ModelPolicy_>
using math_policy_t = typename ModelPolicy_::math_policy;

template <typename ModelPolicy_>
using data_policy_t = typename ModelPolicy_::data_policy;

} // namespace prtcl::rt
