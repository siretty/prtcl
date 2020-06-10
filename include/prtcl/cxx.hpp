#ifndef PRTCL_CXX_HPP
#define PRTCL_CXX_HPP

#include <prtcl/config.hpp>

#include <type_traits>

namespace prtcl::cxx {

#ifdef PRTCL_CXX_HAS_REMOVE_CVREF
template <typename T> using remove_cvref_t = std::remove_cvref_t<T>;
#else
template<typename T> using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#endif

} // namespace prtcl::cxx

#endif //PRTCL_CXX_HPP
