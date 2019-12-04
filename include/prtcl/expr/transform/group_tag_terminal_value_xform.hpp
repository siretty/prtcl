#pragma once

#include <prtcl/expr/transform/xform_helper.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/group.hpp>

namespace prtcl::expr {

struct group_tag_terminal_value_xform : private xform_helper {
  template <typename GT, typename = std::enable_if_t<tag::is_group_v<GT>>>
  term<meta::remove_cvref_t<GT>> operator()(term<GT>) const {
    return {};
  }
};

} // namespace prtcl::expr
