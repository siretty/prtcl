#pragma once

#include <prtcl/meta/is_any_of.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/common.hpp>

#include "_header.hpp"

namespace prtcl::tag {

namespace call {

struct dot {
  PRTCL_DEFINE_TAG_OPERATORS(dot)
};

struct norm {
  PRTCL_DEFINE_TAG_OPERATORS(norm)
};

struct norm_squared {
  PRTCL_DEFINE_TAG_OPERATORS(norm_squared)
};

struct normalized {
  PRTCL_DEFINE_TAG_OPERATORS(normalized)
};

struct min {
  PRTCL_DEFINE_TAG_OPERATORS(min)
};

struct max {
  PRTCL_DEFINE_TAG_OPERATORS(max)
};

struct kernel {
  PRTCL_DEFINE_TAG_OPERATORS(kernel)
};

struct kernel_gradient {
  PRTCL_DEFINE_TAG_OPERATORS(kernel_gradient)
};

} // namespace call

template <> struct is_tag<call::dot> : std::true_type {};
template <> struct is_tag<call::norm> : std::true_type {};
template <> struct is_tag<call::norm_squared> : std::true_type {};
template <> struct is_tag<call::normalized> : std::true_type {};
template <> struct is_tag<call::min> : std::true_type {};
template <> struct is_tag<call::max> : std::true_type {};
template <> struct is_tag<call::kernel> : std::true_type {};
template <> struct is_tag<call::kernel_gradient> : std::true_type {};

template <typename T>
struct is_call
    : meta::is_any_of<meta::remove_cvref_t<T>, call::dot, call::norm,
                      call::norm_squared, call::normalized, call::min,
                      call::max, call::kernel, call::kernel_gradient> {};

template <typename T> constexpr bool is_call_v = is_call<T>::value;

namespace call {

PRTCL_DEFINE_TAG_COMPARISON(::prtcl::tag::is_call)

} // namespace call

} // namespace prtcl::tag

#include "_footer.hpp"
