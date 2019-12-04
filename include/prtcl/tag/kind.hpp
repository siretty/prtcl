#pragma once

#include <prtcl/meta/is_any_of.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/common.hpp>

#include "_header.hpp"

namespace prtcl::tag {

namespace kind {

struct global {
  PRTCL_DEFINE_TAG_OPERATORS(global)
};

struct uniform {
  PRTCL_DEFINE_TAG_OPERATORS(uniform)
};

struct varying {
  PRTCL_DEFINE_TAG_OPERATORS(varying)
};

} // namespace kind

template <> struct is_tag<kind::global> : std::true_type {};
template <> struct is_tag<kind::uniform> : std::true_type {};
template <> struct is_tag<kind::varying> : std::true_type {};

template <typename T>
struct is_kind : meta::is_any_of<meta::remove_cvref_t<T>, kind::global,
                                 kind::uniform, kind::varying> {};

template <typename T> constexpr bool is_kind_v = is_kind<T>::value;

namespace kind {

PRTCL_DEFINE_TAG_COMPARISON(::prtcl::tag::is_kind)

} // namespace kind

} // namespace prtcl::tag

#include "_footer.hpp"
