#pragma once

#include "../tags.hpp"

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename GroupTag, typename Select> struct loop {
  static_assert(tag::is_group_tag_v<GroupTag>, "GroupTag is invalid");

  using group_tag = GroupTag;

  Select select;
};

template <typename GT, typename S>
using loop_term = boost::yap::terminal<boost::yap::expression, loop<GT, S>>;

template <typename S> using active_loop = loop_term<tag::active, S>;
template <typename S> using passive_loop = loop_term<tag::passive, S>;

} // namespace prtcl::expr
