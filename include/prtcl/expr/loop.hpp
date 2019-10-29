#pragma once

#include "../tags.hpp"
#include <prtcl/meta/remove_cvref.hpp>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename GroupTag, typename Select> struct loop {
  static_assert(tag::is_group_tag_v<GroupTag> or
                    tag::is_unspecified_tag_v<GroupTag>,
                "GroupTag is invalid");

  using group_tag = GroupTag;

  Select select;
};

template <typename GT, typename S>
using loop_term = boost::yap::terminal<boost::yap::expression, loop<GT, S>>;

template <typename S> using active_loop = loop_term<tag::active, S>;
template <typename S> using passive_loop = loop_term<tag::passive, S>;

template <typename Select> auto make_active_loop(Select &&select_) {
  return active_loop<meta::remove_cvref_t<Select>>{
      {std::forward<Select>(select_)}};
}

template <typename Select> auto make_passive_loop(Select &&select_) {
  return passive_loop<meta::remove_cvref_t<Select>>{
      {std::forward<Select>(select_)}};
}

template <typename Select> auto make_loop(Select &&select_) {
  return loop_term<tag::unspecified, meta::remove_cvref_t<Select>>{
      {std::forward<Select>(select_)}};
}

} // namespace prtcl::expr
