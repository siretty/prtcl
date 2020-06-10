#ifndef PRTCL_CONFIG_HPP
#define PRTCL_CONFIG_HPP

#define PRTCL_MATH_IMPL_EIGEN 1
#define PRTCL_MATH_IMPL PRTCL_MATH_IMPL_EIGEN

#if __has_include(<version>)

#ifdef __cpp_lib_remove_cvref
#define PRTCL_CXX_HAS_REMOVE_CVREF
#endif

#endif

#endif //PRTCL_CONFIG_HPP
