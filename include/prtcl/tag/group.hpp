#pragma once

#include <prtcl/meta/is_any_of.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/common.hpp>

#include "_header.hpp"

namespace prtcl::tag {

namespace group {

struct active {
  PRTCL_DEFINE_TAG_OPERATORS(active)
};

struct passive {
  PRTCL_DEFINE_TAG_OPERATORS(passive)
};

} // namespace group

template <> struct is_tag<group::active> : std::true_type {};
template <> struct is_tag<group::passive> : std::true_type {};

template <typename T>
struct is_group
    : meta::is_any_of<meta::remove_cvref_t<T>, group::active, group::passive> {
};

template <typename T> constexpr bool is_group_v = is_group<T>::value;

namespace group {

PRTCL_DEFINE_TAG_COMPARISON(::prtcl::tag::is_group)

} // namespace group

} // namespace prtcl::tag

#include "_footer.hpp"
