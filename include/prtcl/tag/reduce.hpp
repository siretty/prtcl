#pragma once

#include <prtcl/meta/is_any_of.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/common.hpp>

#include "_header.hpp"

namespace prtcl::tag {

namespace reduce {

struct plus {
  PRTCL_DEFINE_TAG_OPERATORS(plus)
};

struct minus {
  PRTCL_DEFINE_TAG_OPERATORS(minus)
};

struct multiplies {
  PRTCL_DEFINE_TAG_OPERATORS(multiplies)
};

struct divides {
  PRTCL_DEFINE_TAG_OPERATORS(divides)
};

struct min {
  PRTCL_DEFINE_TAG_OPERATORS(min)
};

struct max {
  PRTCL_DEFINE_TAG_OPERATORS(max)
};

} // namespace reduce

template <> struct is_tag<reduce::plus> : std::true_type {};
template <> struct is_tag<reduce::minus> : std::true_type {};
template <> struct is_tag<reduce::multiplies> : std::true_type {};
template <> struct is_tag<reduce::divides> : std::true_type {};
template <> struct is_tag<reduce::min> : std::true_type {};
template <> struct is_tag<reduce::max> : std::true_type {};

template <typename T>
struct is_reduce : meta::is_any_of<meta::remove_cvref_t<T>, reduce::plus,
                                   reduce::minus, reduce::multiplies,
                                   reduce::divides, reduce::min, reduce::max> {
};

template <typename T> constexpr bool is_reduce_v = is_reduce<T>::value;

namespace reduce {

PRTCL_DEFINE_TAG_COMPARISON(::prtcl::tag::is_reduce)

} // namespace reduce

} // namespace prtcl::tag

#include "_footer.hpp"
