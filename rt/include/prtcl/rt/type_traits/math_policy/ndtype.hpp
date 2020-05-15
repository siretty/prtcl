#pragma once

#include <prtcl/rt/common.hpp>

#include <prtcl/rt/type_traits/is_math_policy.hpp>

#include <type_traits>

namespace prtcl::rt {

template <typename MathPolicy_, dtype DType_, size_t... Ns_>
using ndtype_t = typename MathPolicy_::template ndtype_t<DType_, Ns_...>;

template <typename MathPolicy_, size_t... Ns_>
using nreal_t = ndtype_t<MathPolicy_, dtype::real, Ns_...>;

template <typename MathPolicy_, size_t... Ns_>
using ninteger_t = ndtype_t<MathPolicy_, dtype::integer, Ns_...>;

template <typename MathPolicy_, size_t... Ns_>
using nboolean_t = ndtype_t<MathPolicy_, dtype::boolean, Ns_...>;

} // namespace prtcl::rt
