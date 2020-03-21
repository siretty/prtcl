#pragma once

#include <prtcl/rt/common.hpp>

#include <cstddef>

namespace prtcl::rt {

template <typename MathPolicy_, size_t... Ns_>
using nd_real_t = typename MathPolicy_::template ndtype_t<dtype::real, Ns_...>;

template <typename MathPolicy_, size_t... Ns_>
using nd_integer_t =
    typename MathPolicy_::template ndtype_t<dtype::integer, Ns_...>;

template <typename MathPolicy_, size_t... Ns_>
using nd_boolean_t =
    typename MathPolicy_::template ndtype_t<dtype::boolean, Ns_...>;

} // namespace prtcl::rt
