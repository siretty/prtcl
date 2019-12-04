#pragma once

#include <type_traits>

#include <prtcl/meta/remove_cvref.hpp>

namespace prtcl::tag {

template <typename> struct is_tag : std::false_type {};

template <typename T> constexpr bool is_tag_v = is_tag<T>::value;

} // namespace prtcl::tag
