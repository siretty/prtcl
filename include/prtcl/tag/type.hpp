#pragma once

#include <prtcl/meta/is_any_of.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/common.hpp>

#include "_header.hpp"

namespace prtcl::tag {

namespace type {

struct scalar {
  PRTCL_DEFINE_TAG_OPERATORS(scalar)
};

struct vector {
  PRTCL_DEFINE_TAG_OPERATORS(vector)
};

struct matrix {
  PRTCL_DEFINE_TAG_OPERATORS(matrix)
};

} // namespace type

template <> struct is_tag<type::scalar> : std::true_type {};
template <> struct is_tag<type::vector> : std::true_type {};
template <> struct is_tag<type::matrix> : std::true_type {};

template <typename T>
struct is_type : meta::is_any_of<meta::remove_cvref_t<T>, type::scalar,
                                 type::vector, type::matrix> {};

template <typename T> constexpr bool is_type_v = is_type<T>::value;

namespace type {

PRTCL_DEFINE_TAG_COMPARISON(::prtcl::tag::is_type)

} // namespace type

} // namespace prtcl::tag

#include "_footer.hpp"
