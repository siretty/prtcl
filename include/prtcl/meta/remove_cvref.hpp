#pragma once

#include <type_traits>

namespace prtcl::meta {

template <typename T>
struct remove_cvref : std::remove_cv<std::remove_reference_t<T>> {};

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

} // namespace prtcl::meta

namespace prtcl {

using ::prtcl::meta::remove_cvref;
using ::prtcl::meta::remove_cvref_t;

} // namespace prtcl
