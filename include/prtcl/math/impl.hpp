#ifndef PRTCL_MATH_IMPL_HPP
#define PRTCL_MATH_IMPL_HPP

#include <prtcl/config.hpp>

#if PRTCL_MATH_IMPL == PRTCL_MATH_IMPL_EIGEN

#include "eigen.hpp"

#else
#error "no math implementation selected"
#endif

#endif // PRTCL_MATH_IMPL_HPP
