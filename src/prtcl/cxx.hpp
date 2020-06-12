#ifndef PRTCL_CXX_HPP
#define PRTCL_CXX_HPP

#include <prtcl/config.hpp>

#include <type_traits>

#include <cstddef>

#define TCB_SPAN_NAMESPACE_NAME prtcl::cxx
#include "cxx/span.inc"

// export / import from https://stackoverflow.com/a/2164853/9686644
#if defined(_MSC_VER)
#define PRTCL_EXPORT __declspec(dllexport)
#define PRTCL_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
#define PRTCL_EXPORT __attribute__((visibility("default")))
#define PRTCL_IMPORT
#else
#define PRTCL_EXPORT
#define PRTCL_IMPORT
#pragma warning "Unknown dynamic link import/export semantics."
#endif

namespace prtcl::cxx {

#ifdef PRTCL_CXX_HAS_REMOVE_CVREF
template <typename T> using remove_cvref_t = std::remove_cvref_t<T>;
#else
template<typename T> using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#endif

} // namespace prtcl::cxx

#endif //PRTCL_CXX_HPP
