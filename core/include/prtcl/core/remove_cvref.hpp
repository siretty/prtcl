#pragma once

#include <type_traits>

namespace prtcl::core {

template <typename T>
struct remove_cvref : std::remove_cv<std::remove_reference_t<T>> {};

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

} // namespace prtcl::core
