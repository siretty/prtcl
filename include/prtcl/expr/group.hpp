#pragma once

#include "../tags.hpp"

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename GroupTag> struct group {
  static_assert(tag::is_group_tag_v<GroupTag>, "GroupTag is invalid");

  using group_tag = GroupTag;
};

template <typename GT>
using group_term = boost::yap::terminal<boost::yap::expression, group<GT>>;

template <typename GT> auto term(group<GT> group_) {
  return group_term<GT>{group_};
}

using active_group = group_term<tag::active>;
using passive_group = group_term<tag::passive>;

} // namespace prtcl::expr
